/**
 * @file epoller.h
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#if !defined(EPOLLER_H)
#define EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller
{
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);
    int Wait(int timeoutMs = -1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};

#endif // EPOLLER_H
