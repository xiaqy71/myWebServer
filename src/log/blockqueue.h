/**
 * @file blockqueue.h
 * @author xiaqy (792155443@qq.com)
 * @brief 阻塞队列实现
 * @version 0.1
 * @date 2024-11-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#if !defined(BLOCKQUEUE_H)
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <assert.h>

template <class T> class BlockDeque
{
public:
    explicit BlockDeque(size_t MaxCapcity = 1000);

    ~BlockDeque();

    void clear();

    bool empty();

    bool full();

    void Close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T &item);

    void push_front(const T &item);

    bool pop(T &item);

    bool pop(T &item, int timeout);

    void flush();

private:
    std::deque<T> deq_;

    size_t capacity_;

    std::mutex mtx_;

    bool isClose_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;
};

/**
 * @brief Construct a new Block Deque< T>:: Block Deque object
 *
 * @tparam T 容纳的数据类型
 * @param MaxCapcity 最大容量
 */
template <class T>
inline BlockDeque<T>::BlockDeque(size_t MaxCapcity)
: capacity_(MaxCapcity)
{
    assert(MaxCapcity > 0);
    isClose_ = false;
}

/**
 * @brief Destroy the Block Deque< T>:: Block Deque object
 *
 * @tparam T
 */
template <class T> inline BlockDeque<T>::~BlockDeque() { Close(); }

/**
 * @brief 清空队列
 *
 * @tparam T
 */
template <class T> inline void BlockDeque<T>::clear()
{
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

/**
 * @brief 判断队列是否为空
 *
 * @tparam T
 * @return true 队列为空
 * @return false 队列非空
 */
template <class T> inline bool BlockDeque<T>::empty()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

/**
 * @brief 判断队列是否已满
 *
 * @tparam T
 * @return true 队列已满
 * @return false 队列未满
 */
template <class T> inline bool BlockDeque<T>::full()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

/**
 * @brief 关闭队列
 *
 * @tparam T
 */
template <class T> inline void BlockDeque<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
}

/**
 * @brief 获取队列大小
 *
 * @tparam T
 * @return size_t 队列大小
 */
template <class T> inline size_t BlockDeque<T>::size()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

/**
 * @brief 获取队列容量
 *
 * @tparam T
 * @return size_t 队列容量
 */
template <class T> inline size_t BlockDeque<T>::capacity()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

/**
 * @brief 获取队首元素
 *
 * @tparam T
 * @return T 队首元素
 */
template <class T> inline T BlockDeque<T>::front()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

/**
 * @brief 获取队尾元素
 *
 * @tparam T
 * @return T 队尾元素
 */
template <class T> inline T BlockDeque<T>::back()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

/**
 * @brief 追加元素到队尾
 *
 * @tparam T
 * @param item 待追加的元素
 */
template <class T> inline void BlockDeque<T>::push_back(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
}

/**
 * @brief 追加元素到队首
 *
 * @tparam T
 * @param item 待追加的元素
 */
template <class T> inline void BlockDeque<T>::push_front(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

/**
 * @brief 弹出队首元素，阻塞等待
 *
 * @tparam T
 * @param item
 * @return true
 * @return false
 */
template <class T> inline bool BlockDeque<T>::pop(T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty())
    {
        condConsumer_.wait(locker);
        if (isClose_)
        {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

/**
 * @brief 弹出队首元素，超时返回
 *
 * @tparam T
 * @param item
 * @param timeout 超时时间
 * @return true
 * @return false
 */
template <class T> inline bool BlockDeque<T>::pop(T &item, int timeout)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty())
    {
        if (condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) ==
            std::cv_status::timeout)
        {
            return false;
        }
        if (isClose_)
        {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

/**
 * @brief 刷新队列
 *
 * @tparam T
 */
template <class T> inline void BlockDeque<T>::flush()
{
    condConsumer_.notify_one();
}

#endif // BLOCKQUEUE_H
