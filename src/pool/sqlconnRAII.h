/**
 * @file sqlconnRAII.h
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#if !defined(SQLCONNRAII_H)
#define SQLCONNRAII_H

#include "sqlconnpool.h"

class SqlConnRAII
{
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connPool)
    {
        *sql = connPool->GetConn();
        sql_ = *sql;
        connPool_ = connPool;
    }

    ~SqlConnRAII()
    {
        if (sql_)
        {
            connPool_->returnConn(sql_);
        }
    }

private:
    MYSQL* sql_;
    SqlConnPool *connPool_;
};

#endif // SQLCONNRAII_H
