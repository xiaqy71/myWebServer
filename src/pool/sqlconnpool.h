/**
 * @file sqlconnpool.h
 * @author xiaqy (792155443@qq.com)
 * @brief 数据库连接池声明
 * @version 0.1
 * @date 2024-11-06
 *
 * @copyright Copyright (c) 2024
 *
 */

#if !defined(SQLCONNPOOL_H)
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <memory>
#include "log.h"

/**
 * @brief 数据库异常类
 *
 */
class SqlConnPoolException : public std::exception
{
public:
    SqlConnPoolException(const char *msg)
    : msg_(msg)
    {
    }
    const char *what() const noexcept override { return msg_.c_str(); }

private:
    std::string msg_;
};

class SqlConnPool
{
    friend class SqlConnRAII;

public:
    static SqlConnPool *Instance();

    int GetFreeConnCount();

    void Init(const char *host,
              int port,
              const char *user,
              const char *pwd,
              const char *dbName,
              int connSize);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    MYSQL *GetConn();
    void returnConn(MYSQL *conn);

    int MAX_CONN_;
    // int useCount_;
    // int freeCount_;

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};

#endif // SQLCONNPOOL_H
