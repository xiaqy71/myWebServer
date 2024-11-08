#include <gtest/gtest.h>
#include "sqlconnpool.h"
#include "sqlconnRAII.h"

const int MAX_CONN = 8;

TEST(SqlConnPool_TEST, Init)
{
    auto sqlpool = SqlConnPool::Instance();
    sqlpool->Init("localhost", 3306, "xiaqy", "123456", "WebServer", MAX_CONN);

    MYSQL* sql = nullptr;
    {
        SqlConnRAII con(&sql, sqlpool);
        EXPECT_NE(sql, nullptr);
    }
    EXPECT_EQ(sql, nullptr);
    EXPECT_EQ(sqlpool->GetFreeConnCount(), MAX_CONN);
}

TEST(SqlConnPool_TEST, InitException)
{
    EXPECT_THROW(SqlConnPool::Instance()->Init("localhost", 3306, "xiaqy", "12345", "lalala", MAX_CONN), 
        SqlConnPoolException
    );
}