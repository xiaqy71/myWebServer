/**
 * @file sqlconnpool.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 数据库连接池实现
 * @version 0.1
 * @date 2024-11-06
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "sqlconnpool.h"

SqlConnPool *SqlConnPool::Instance()
{
    static SqlConnPool connPool;
    return &connPool;
}
/**
 * @brief 从连接池中获取一个连接
 *
 * @return MYSQL*
 */
MYSQL *SqlConnPool::GetConn()
{
    MYSQL *sql = nullptr;
    if (connQue_.empty())
    {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

/**
 * @brief 将连接放回连接池
 *
 * @param conn
 */
void SqlConnPool::returnConn(MYSQL *conn)
{
    if (conn == nullptr)
    {
        throw SqlConnPoolException("return null connection");
    }
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);
}

/**
 * @brief 获取空闲连接数
 *
 * @return int
 */
int SqlConnPool::GetFreeConnCount()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}

/**
 * @brief 初始化数据库连接池
 *
 * @param host 主机地址
 * @param port 数据库端口号
 * @param user 用户名
 * @param pwd 密码
 * @param dbName 数据库名
 * @param connSize 连接池大小
 */
void SqlConnPool::Init(const char *host,
                       int port,
                       const char *user,
                       const char *pwd,
                       const char *dbName,
                       int connSize)
{
    if (connSize <= 0)
    {
        // 抛出异常
        throw SqlConnPoolException("connSize <= 0");
    }
    for (int i = 0; i < connSize; ++i)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql)
        {
            // 抛出异常
            throw SqlConnPoolException("mysql_init error");
        }
        sql =
            mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
        if (!sql)
        {
            // 抛出异常
            throw SqlConnPoolException("mysql_real_connect error");
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);

    LOG_INFO("SqlConnPool init success");
}

/**
 * @brief 关闭连接池
 *
 */
void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty())
    {
        auto sql = connQue_.front();
        connQue_.pop();
        // mysql_close(sql.get());
    }
    mysql_library_end();
}

SqlConnPool::SqlConnPool() { }

SqlConnPool::~SqlConnPool()
{
    ClosePool();
    sem_destroy(&semId_);
}
