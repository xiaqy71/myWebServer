/**
 * @file httpconn.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "httpconn.h"

const char *HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn()
: fd_(-1)
, addr_({0})
, isClose_(true)
{
}

HttpConn::~HttpConn() { Close(); }

/**
 * @brief 初始化http连接
 * 
 * @param sockFd 
 * @param addr 
 */
void HttpConn::init(int sockFd, const sockaddr_in &addr)
{
    if (sockFd < 0)
    {
        LOG_ERROR("invalid socket fd");
        throw std::invalid_argument("invalid socket fd");
    }
    userCount++;
    addr_ = addr;
    fd_ = sockFd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d",
             fd_,
             getIP(),
             getPort(),
             static_cast<int>(userCount));
}

/**
 * @brief 读取http连接的数据
 * 
 * @param saveErrno 
 * @return ssize_t 
 */
ssize_t HttpConn::read(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0)
        {
            break;
        }
    } while (isET);
    return len;
}

/**
 * @brief 写入http连接的数据
 * 
 * @param saveErrno 
 * @return ssize_t 
 */
ssize_t HttpConn::write(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0)
        {
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0)
        {
            /* 传输结束 */
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len)
        {
            iov_[1].iov_base =
                static_cast<char *>(iov_[1].iov_base) + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len)
            {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else
        {
            iov_[0].iov_base = static_cast<char *>(iov_[0].iov_base) + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

/**
 * @brief 关闭http连接
 * 
 */
void HttpConn::Close()
{
    response_.UnmapFile();
    if (isClose_ == false)
    {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d",
                 fd_,
                 getIP(),
                 getPort(),
                 static_cast<int>(userCount));
    }
}

/**
 * @brief 获取该http连接的描述符
 * 
 * @return int 
 */
int HttpConn::getFd() const { return fd_; }

/**
 * @brief 获取该http连接的端口
 * 
 * @return int 
 */
int HttpConn::getPort() const { return addr_.sin_port; }

/**
 * @brief 获取该http连接的ip地址
 * 
 * @return const char* 
 */
const char *HttpConn::getIP() const { return inet_ntoa(addr_.sin_addr); }

/**
 * @brief 获取该http连接的地址
 * 
 * @return sockaddr_in 
 */
sockaddr_in HttpConn::getAddr() const { return addr_; }

/**
 * @brief 处理http连接
 * 
 * @return true 
 * @return false 
 */
bool HttpConn::process()
{
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuff_))
    {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else
    {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if (response_.FileLen() > 0 && response_.File())
    {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG(
        "filesize:%d, %d to %d", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
}
