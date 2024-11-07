/**
 * @file log.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 简易日志系统实现
 * @version 0.1
 * @date 2024-11-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "log.h"

/**
 * @brief 初始化日志系统
 *
 * @param level 日志等级
 * @param path 日志路径
 * @param suffix 日志后缀
 * @param maxQueueCapacity 队列容量
 */
void Log::init(LogLevel level,
               const char *path,
               const char *suffix,
               int maxQueueCapacity)
{
    isOpen_ = true;
    level_ = level;
    /* 队列容量大于0，则为异步模式 */
    if (maxQueueCapacity > 0)
    {
        isAsync_ = true;
        if (!deque_)
        {
            deque_ = std::make_unique<BlockDeque<std::string>>();
            writeThread_ = std::make_unique<std::thread>(FlushLogThread);
        }
    }
    else
    {
        isAsync_ = false;
    }

    lineCount_ = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName,
             LOG_NAME_LEN - 1,
             "%s/%04d_%02d_%02d%s",
             path_,
             t.tm_year + 1900,
             t.tm_mon + 1,
             t.tm_mday,
             suffix_);
    toDay_ = t.tm_mday;
    {
        /* 加锁， 避免多个线程同时操作fp_指针 */
        std::lock_guard<std::mutex> locker(mtx_);
        buff_.RetrieveAll();
        if (fp_)
        {
            /* 唤醒线程 */
            flush();
            fclose(fp_);
        }

        fp_ = fopen(fileName, "a");
        /*打开失败， 则先创建目录*/
        if (fp_ == nullptr)
        {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

/**
 * @brief 获取日志单例
 * 
 * @return Log* 
 */
Log *Log::Instance()
{
    static Log inst;
    return &inst;
}

/**
 * @brief 刷新日志线程
 *
 */
void Log::FlushLogThread() { Log::Instance()->AsyncWrite_(); }

/**
 * @brief 写入日志的主要函数
 *
 * @param level 日志等级
 * @param format 日志格式
 * @param ... 可变参数
 */
void Log::write(LogLevel level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail,
                 36,
                 "%04d_%02d_%02d",
                 t.tm_year + 1900,
                 t.tm_mon + 1,
                 t.tm_mday);

        if (toDay_ != t.tm_mday)
        {
            snprintf(
                newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else
        {
            snprintf(newFile,
                     LOG_NAME_LEN - 72,
                     "%s/%s-%d%s",
                     path_,
                     tail,
                     (lineCount_ / MAX_LINES),
                     suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(mtx_);
        lineCount_++;
        int n = snprintf(buff_.BeginWrite(),
                         128,
                         "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900,
                         t.tm_mon + 1,
                         t.tm_mday,
                         t.tm_hour,
                         t.tm_min,
                         t.tm_sec,
                         now.tv_usec);

        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        int m = vsnprintf(
            buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full())
        {
            deque_->push_back(buff_.RetrieveAllToStr());
        }
        else
        {
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }
}

/**
 * @brief 刷新日志
 *
 */
void Log::flush()
{
    if (isAsync_)
    {
        deque_->flush();
    }
    fflush(fp_);
}

/**
 * @brief 获取日志等级
 *
 * @return int 日志等级
 */
LogLevel Log::GetLevel()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

/**
 * @brief 设置日志等级
 *
 * @param level 日志等级
 */
void Log::SetLevel(LogLevel level)
{
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

/**
 * @brief Construct a new Log:: Log object
 *
 */
Log::Log()
{
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

/**
 * @brief 向日志添加头部，如[debug]:
 *
 * @param level 日志等级
 */
void Log::AppendLogLevelTitle_(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        buff_.Append("[debug]: ", 9);
        break;
    case LogLevel::INFO:
        buff_.Append("[info]: ", 8);
        break;
    case LogLevel::WARN:
        buff_.Append("[warn]: ", 8);
        break;
    case LogLevel::ERROR:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info]: ", 8);
        break;
    }
}

/**
 * @brief Destroy the Log:: Log object
 *
 */
Log::~Log()
{
    if (writeThread_ && writeThread_->joinable())
    {
        while (!deque_->empty())
        {
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();
    }

    if (fp_)
    {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

/**
 * @brief 异步写入日志
 *
 */
void Log::AsyncWrite_()
{
    std::string str = "";
    while (deque_->pop(str))
    {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}
