/**
 * @file httpconn.h
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#if !defined(HTTP_CONN_H)
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#include "log.h"
#include "sqlconnRAII.h"
#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    void init(int sockFd, const sockaddr_in &addr);
    ssize_t read(int *saveErrno);
    ssize_t write(int *saveErrno);

    void Close();
    int getFd() const;
    int getPort() const;
    const char *getIP() const;
    sockaddr_in getAddr() const;

    bool process();
    int ToWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }
    bool isKeepAlive() const { return request_.IsKeepAlive(); }

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;
    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;  // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

#endif // HTTP_CONN_H
