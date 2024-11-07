/**
 * @file epoller.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "epoller.h"

Epoller::Epoller(int maxEvent)
: epollFd_(epoll_create(512))
, events_(maxEvent)
{
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() { close(epollFd_); }

/**
 * @brief 向epoll内核事件表中添加文件描述符
 * 
 * @param fd 
 * @param events 
 * @return true 
 * @return false 
 */
bool Epoller::AddFd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

/**
 * @brief 修改epoll内核事件表中的文件描述符
 * 
 * @param fd 
 * @param events 
 * @return true 
 * @return false 
 */
bool Epoller::ModFd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

/**
 * @brief 从epoll内核事件表中删除文件描述符
 * 
 * @param fd 
 * @return true 
 * @return false 
 */
bool Epoller::DelFd(int fd)
{
    if (fd < 0)
        return false;
    struct epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

/**
 * @brief 等待事件的发生
 * 
 * @param timeoutMs 
 * @return int 
 */
int Epoller::Wait(int timeoutMs)
{
    return epoll_wait(
        epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

/**
 * @brief 获取事件的文件描述符
 * 
 * @param i 
 * @return int 
 */
int Epoller::GetEventFd(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

/**
 * @brief 获取事件的类型
 * 
 * @param i 
 * @return uint32_t 
 */
uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}
