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
#include <tuple>
#include "configMgr.h"
#include "webserver.h"

int main(int argc, char const *argv[])
{
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
