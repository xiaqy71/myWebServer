/**
 * @file heaptimer.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 基于小根堆的定时器实现
 * @version 0.1
 * @date 2024-11-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "heaptimer.h"

/**
 * @brief 删除指定index位置的节点
 *
 * @param index 待删除节点的位置
 */
void HeapTimer::del_(size_t index)
{
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n)
    {
        SwapNode_(i, n);
        if (!siftdown_(i, n))
        {
            siftup_(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

/**
 * @brief 向上调整堆
 *
 * @param i 调整的节点位置
 */
void HeapTimer::siftup_(size_t i)
{
    if (i == 0)
    {
        return;
    }
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;

    while (j >= 0)
    {
        if (heap_[j] < heap_[i])
        {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

/**
 * @brief 向下调整堆
 *
 * @param index 待调整节点的位置
 * @param n 堆的有效大小
 * @return true 调整成功
 * @return false 调整失败
 */
bool HeapTimer::siftdown_(size_t index, size_t n)
{
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n)
    {
        if (j + 1 < n && heap_[j + 1] < heap_[j])
            ++j;
        if (heap_[i] < heap_[j])
            break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

/**
 * @brief 交换两个节点
 *
 * @param i
 * @param j
 */
void HeapTimer::SwapNode_(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

/**
 * @brief 调整指定id节点的超时时间
 *
 * @param id 节点id
 * @param newExpires 新的超时时间
 */
void HeapTimer::adjust(int id, int newExpires)
{
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(newExpires);
    siftdown_(ref_[id], heap_.size());
}

/**
 * @brief 添加新的超时节点
 *
 * @param id
 * @param timeOut
 * @param cb 超时回调函数
 */
void HeapTimer::add(int id, int timeOut, const TimeOutCallBack &cb)
{
    assert(id >= 0);
    size_t i;
    if (ref_.count(id) == 0)
    {
        // 新节点：堆尾插入， 调整堆
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeOut), cb});
        siftup_(i);
    }
    else
    {
        /* 已有节点： 调整堆*/
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeOut);
        heap_[i].cb = cb;
        if (!siftdown_(i, heap_.size()))
        {
            siftup_(i);
        }
    }
}

/**
 * @brief 删除指定id节点，并调用回调函数
 *
 * @param id 待删除节点的id
 */
void HeapTimer::doWork(int id)
{
    if (heap_.empty() || ref_.count(id) == 0)
    {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

/**
 * @brief 清空堆
 *
 */
void HeapTimer::clear()
{
    ref_.clear();
    heap_.clear();
}

/**
 * @brief 超时处理函数
 *
 */
void HeapTimer::tick()
{
    if (heap_.empty())
    {
        return;
    }
    while (!heap_.empty())
    {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now())
                .count() > 0)
        {
            break;
        }
        node.cb();
        pop();
    }
}

/**
 * @brief 删除堆顶节点
 *
 */
void HeapTimer::pop()
{
    assert(!heap_.empty());
    del_(0);
}

/**
 * @brief 获取下一个超时节点的超时时间
 *
 * @return int 应该设置的定时器超时时间
 */
int HeapTimer::GetNextTick()
{
    tick();
    size_t res = -1;
    if (!heap_.empty())
    {
        res =
            std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now())
                .count();
        if (res < 0)
        {
            res = 0;
        }
    }

    return res;
}
