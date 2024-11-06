/**
 * @file main.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "log.h"
#include "configMgr.h"
#include "threadpool.h"
#include "sqlconnpool.h"
#include "sqlconnRAII.h"
int main(int argc, char const *argv[])
{
    /* 初始化日志系统 */
    Log::Instance()->init(LogLevel::DEBUG, "./log", ".log", 1024);
    /* 从配置文件读取配置 */
    assert(configMgr::Instance().init("config.ini"));
    
    // 数据库连接池测试
    SqlConnPool::Instance().Init("127.0.0.1", 3306, "xiaqy", "123456", "WebServer", 8);

    return 0;
}
