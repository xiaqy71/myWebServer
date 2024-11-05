#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

size_t Buffer::WritableBytes() const
{
    return buffer_.size() - writePos_;
}

size_t Buffer::ReadableBytes() const
{
    return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes() const
{
    return readPos_;
}

const char *Buffer::Peek() const
{
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWriteable(size_t len)
{
    if (WritableBytes() < len)
    {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

void Buffer::HasWritten(size_t len)
{
    writePos_ += len;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char *end)
{
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetriveAll()
{
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr()
{
    std::string str(Peek(), ReadableBytes());
    RetriveAll();
    return str;
}

const char *Buffer::BeginWriteConst() const
{
    return const_cast<Buffer *>(this)->BeginWrite();
}

char *Buffer::BeginWrite()
{
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
}

void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const void *data, size_t len)
{
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const Buffer &buff)
{
    Append(buff.Peek(), buff.ReadableBytes());
}

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

char *Buffer::BeginPtr_()
{
    return &*buffer_.begin();
}

const char *Buffer::BeginPtr_() const
{
    return const_cast<Buffer *>(this)->BeginPtr_();
}

void Buffer::MakeSpace_(size_t len)
{
    if (WritableBytes() + PrependableBytes() < len)
    {
        buffer_.resize(writePos_ + len + 1);
    }
    else
    {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}
