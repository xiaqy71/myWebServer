/**
 * @file buffer.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 缓冲区实现
 * @version 0.1
 * @date 2024-11-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "buffer.h"

/**
 * @brief Construct a new Buffer:: Buffer object
 *
 * @param initBuffSize 初始化缓冲区大小
 */
Buffer::Buffer(int initBuffSize)
: buffer_(initBuffSize)
, readPos_(0)
, writePos_(0)
{
}

/**
 * @brief 可写大小
 *
 * @return size_t 缓冲区可写的字节数
 */
size_t Buffer::WritableBytes() const { return buffer_.size() - writePos_; }

/**
 * @brief 可读大小
 *
 * @return size_t 缓冲区可读的字节数
 */
size_t Buffer::ReadableBytes() const { return writePos_ - readPos_; }

/**
 * @brief 缓冲区头部大小
 *
 * @return size_t 缓冲区头部大小
 */
size_t Buffer::PrependableBytes() const { return readPos_; }

/**
 * @brief 获取缓冲区待读数据的起始位置
 *
 * @return const char* 缓冲区待读数据的起始位置
 */
const char *Buffer::Peek() const { return BeginPtr_() + readPos_; }

/**
 * @brief 确保缓冲区有足够的空间
 *
 * @param len 需要的空间大小
 */
void Buffer::EnsureWriteable(size_t len)
{
    if (WritableBytes() < len)
    {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

/**
 * @brief 已经写入的数据大小
 *
 * @param len 已经写入的数据大小
 */
void Buffer::HasWritten(size_t len) { writePos_ += len; }

/**
 * @brief 偏移已读数据
 *
 * @param len 偏移大小
 */
void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    readPos_ += len;
}

/**
 * @brief 读取数据直到end
 *
 * @param end 读取数据直到end
 */
void Buffer::RetrieveUntil(const char *end)
{
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

/**
 * @brief 将缓冲区中的数据全部视为已读
 *
 */
void Buffer::RetrieveAll()
{
    // 缓冲区全部置零
    memset(buffer_.data(), 0, buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

/**
 * @brief 将缓冲区中的数据全部视为已读，并返回字符串
 *
 * @return std::string 缓冲区中的数据
 */
std::string Buffer::RetrieveAllToStr()
{
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

/**
 * @brief 获取缓冲区待写数据的起始位置
 *
 * @return const char* 缓冲区待写数据的起始位置
 */
const char *Buffer::BeginWriteConst() const
{
    return const_cast<Buffer *>(this)->BeginWrite();
}

/**
 * @brief 获取缓冲区待写数据的起始位置
 *
 * @return char* 缓冲区待写数据的起始位置
 */
char *Buffer::BeginWrite() { return BeginPtr_() + writePos_; }

/**
 * @brief 追加字符串
 *
 * @param str 字符串
 */
void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
}

/**
 * @brief 追加字符串
 *
 * @param str 待追加字符串
 * @param len 待追加字符串长度
 */
void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    /* 确保有足够空间 */
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

/**
 * @brief 追加数据
 *
 * @param data 待追加数据
 * @param len 待追加数据长度
 */
void Buffer::Append(const void *data, size_t len)
{
    Append(static_cast<const char *>(data), len);
}

/**
 * @brief 追加缓冲区
 *
 * @param buff 待追加缓冲区
 */
void Buffer::Append(const Buffer &buff)
{
    Append(buff.Peek(), buff.ReadableBytes());
}

/**
 * @brief 从fd中读取数据
 *
 * @param fd 文件描述符
 * @param Errno 错误码
 * @return ssize_t
 */
ssize_t Buffer::ReadFd(int fd, int *Errno)
{
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *Errno = errno;
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        writePos_ += len;
    }
    else
    {
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }

    return len;
}

/**
 * @brief 将数据写入fd
 *
 * @param fd 文件描述符
 * @param Errno 错误码
 * @return ssize_t
 */
ssize_t Buffer::WriteFd(int fd, int *Errno)
{
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0)
    {
        *Errno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

/**
 * @brief 获取缓冲区起始位置
 *
 * @return char* 缓冲区起始位置
 */
char *Buffer::BeginPtr_() { return buffer_.data(); }

/**
 * @brief 获取缓冲区起始位置
 *
 * @return const char* 缓冲区起始位置
 */
const char *Buffer::BeginPtr_() const
{
    return const_cast<Buffer *>(this)->BeginPtr_();
}

/**
 * @brief 重新分配缓冲区大小
 *
 * @param len 缓冲区大小
 */
void Buffer::MakeSpace_(size_t len)
{
    // 如果可写区域+头部区域小于len, 则重新分配
    if (WritableBytes() + PrependableBytes() < len)
    {
        buffer_.resize(writePos_ + len + 1);
    }
    else
    {
        size_t readable = ReadableBytes();
        // 将可读数据移动到缓冲区头部
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}
