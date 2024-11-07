/**
 * @file threadpool.h
 * @author xiaqy (792155443@qq.com)
 * @brief 线程池实现
 * @version 0.1
 * @date 2024-11-06
 *
 * @copyright Copyright (c) 2024
 *
 */
#if !defined(THREADPOOL_H)
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <assert.h>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadNumber = 8)
    : pool_(std::make_shared<Pool>())
    {
        assert(threadNumber > 0);
        for (size_t i = 0; i < threadNumber; ++i)
        {
            /* 创建threadNumber个线程 并分离*/
            std::thread([pool = pool_] {
                while (true)
                {
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    if (!pool->tasks.empty())
                    {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if (pool->isClosed)
                        break;
                    else
                        pool->cond.wait(locker);
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool &&) = default;

    ~ThreadPool()
    {
        if (static_cast<bool>(pool_))
        {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template <typename F> void AddTask(F &&task)
    {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

#endif // THREADPOOL_H
