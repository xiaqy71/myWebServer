/**
 * @file webserver.h
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#if !defined(WEBSERVER_H)
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <tuple>

#include "epoller.h"
#include "log.h"
#include "heaptimer.h"
#include "sqlconnpool.h"
#include "threadpool.h"
#include "sqlconnRAII.h"
#include "httpconn.h"
#include "configMgr.h"

class WebServer
{
public:
    WebServer(int port,
              int trigMode,
              int timeoutMS,
              bool OptLinger,
              int sqlPort,
              const char *sqlUser,
              const char *sqlPwd,
              const char *dbName,
              int connPoolNum,
              int threadNum,
              bool openLog,
              LogLevel logLevel,
              int logQueSize);

    ~WebServer();

    static std::tuple<int,
                      int,
                      int,
                      bool,
                      int,
                      const char *,
                      const char *,
                      const char *,
                      int,
                      int,
                      bool,
                      LogLevel,
                      int>
    getServerConfig();

    void Start();

private:
    bool InitSocket_();

    void InitEventMode_(int trigMode);

    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();

    void DealWrite_(HttpConn *client);

    void DealRead_(HttpConn *client);

    void SendError_(int fd, const char *info);

    void ExtentTime_(HttpConn *client);

    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);

    void OnWrite_(HttpConn *client);

    void OnProcess(HttpConn *client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    char *srcDir_;
    uint32_t listenEvent_;
    uint32_t connEvent_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};

#endif // WEBSERVER_H
