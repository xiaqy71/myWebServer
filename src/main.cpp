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
#include <unistd.h>
#include "webserver.h"

int main(int argc, char const *argv[])
{
    /* 守护进程 后台运行 */
    //daemon(1, 0);

    /* 读取配置文件 */
    auto [port,
          trigMode,
          timeoutMS,
          OptLinger,
          sqlPort,
          sqlUser,
          sqlPwd,
          dbName,
          connPoolNum,
          threadNum,
          openLog,
          logLevel,
          logQueSize] = WebServer::getServerConfig();

    WebServer server(port,
                     trigMode,
                     timeoutMS,
                     OptLinger,
                     sqlPort,
                     sqlUser,
                     sqlPwd,
                     dbName,
                     connPoolNum,
                     threadNum,
                     openLog,
                     logLevel,
                     logQueSize);
    server.Start();
    return 0;
}
