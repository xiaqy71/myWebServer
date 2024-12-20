/**
 * @file log.h
 * @author xiaqy (792155443@qq.com)
 * @brief 日志系统声明
 * @version 0.1
 * @date 2024-11-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#if !defined(LOG_H)
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"

enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Log
{
public:
    void init(LogLevel level,
              const char *path = "./log",
              const char *suffix = ".log",
              int maxQueueCapacity = 1024);
    static Log *Instance();
    static void FlushLogThread();

    void write(LogLevel level, const char *format, ...);
    void flush();

    LogLevel GetLevel();
    void SetLevel(LogLevel level);
    bool IsOpen() { return isOpen_; }

private:
    Log();
    virtual ~Log();
    void AppendLogLevelTitle_(LogLevel level);
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char *path_;   // 日志路径
    const char *suffix_; // 日志后缀

    int MAX_LINES_; // 日志行数上限

    int lineCount_; // 当前行数
    int toDay_;     // 当前日期

    bool isOpen_; // 是否打开日志

    Buffer buff_;    // 缓冲区
    LogLevel level_; // 日志等级
    bool isAsync_;   // 是否异步

    FILE *fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_; // 队列存放待写入的日志
    std::unique_ptr<std::thread> writeThread_; // 将日志写入文件的线程
    std::mutex mtx_;                           // 互斥锁
};

#define LOG_BASE(level, format, ...)                                           \
    do                                                                         \
    {                                                                          \
        auto log = Log::Instance();                                            \
        if (log->IsOpen() && log->GetLevel() <= level)                         \
        {                                                                      \
            log->write(level, format, ##__VA_ARGS__);                          \
            log->flush();                                                      \
        }                                                                      \
    } while (0);

#define LOG_DEBUG(format, ...)                                                 \
    do                                                                         \
    {                                                                          \
        LOG_BASE(LogLevel::DEBUG, format, ##__VA_ARGS__)                       \
    } while (0);
#define LOG_INFO(format, ...)                                                  \
    do                                                                         \
    {                                                                          \
        LOG_BASE(LogLevel::INFO, format, ##__VA_ARGS__)                        \
    } while (0);
#define LOG_WARN(format, ...)                                                  \
    do                                                                         \
    {                                                                          \
        LOG_BASE(LogLevel::WARN, format, ##__VA_ARGS__)                        \
    } while (0);
#define LOG_ERROR(format, ...)                                                 \
    do                                                                         \
    {                                                                          \
        LOG_BASE(LogLevel::ERROR, format, ##__VA_ARGS__)                       \
    } while (0);

#endif // LOG_H
