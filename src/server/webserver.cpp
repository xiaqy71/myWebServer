/**
 * @file webserver.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "webserver.h"

/**
 * @brief Construct a new Web Server:: Web Server object
 * 
 * @param port 
 * @param trigMode 
 * @param timeoutMS 
 * @param OptLinger 
 * @param sqlPort 
 * @param sqlUser 
 * @param sqlPwd 
 * @param dbName 
 * @param connPoolNum 
 * @param threadNum 
 * @param openLog 
 * @param logLevel 
 * @param logQueSize 
 */
WebServer::WebServer(int port,
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
                     int logQueSize)
: port_(port)
, openLinger_(OptLinger)
, timeoutMS_(timeoutMS)
, isClose_(false)
, timer_(new HeapTimer())
, epoller_(new Epoller())
, threadpool_(new ThreadPool(threadNum))
{
    /* 解析resouces目录位置*/
    char exePath[256] = {0};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    exePath[len] = '\0';
    auto dirPath =
        std::string(exePath).substr(0, std::string(exePath).find_last_of('/')) +
        "/resources/";
    srcDir_ = new char[dirPath.size() + 1];
    memcpy(srcDir_, dirPath.c_str(), dirPath.size() + 1);
    // 初始化用户数
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    // 初始化数据库连接池
    SqlConnPool::Instance()->Init(
        "localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_(trigMode);
    if (!InitSocket_())
    {
        isClose_ = true;
    }

    if (openLog)
    {
        char exePath[256] = {0};
        ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
        exePath[len] = '\0';
        auto dirPath = std::string(exePath).substr(
            0, std::string(exePath).find_last_of('/'));
        Log::Instance()->init(
            logLevel, (dirPath + "/log").c_str(), ".log", logQueSize);
        if (isClose_)
        {
            LOG_ERROR("========== Server init error ==========");
        }
        else
        {
            LOG_INFO("========== Server init ==========");
            LOG_INFO(
                "Port: %d, OpenLinger: %s", port, OptLinger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET ? "ET" : "LT"),
                     (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d",
                     connPoolNum,
                     threadNum);
        }
    }
}

/**
 * @brief Destroy the Web Server:: Web Server object
 * 
 */
WebServer::~WebServer()
{
    close(listenFd_);
    isClose_ = true;
    delete[] srcDir_;
    SqlConnPool::Instance()->ClosePool();
}

std::tuple<int,
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
WebServer::getServerConfig()
{
    auto &cfg = configMgr::Instance();
    cfg.init("config.ini");

    int port = std::stoi(static_cast<const char *>(cfg["server"]["port"]));
    int trigMode =
        std::stoi(static_cast<const char *>(cfg["server"]["trigMode"]));
    int timeoutMS =
        std::stoi(static_cast<const char *>(cfg["server"]["timeoutMS"]));
    bool OptLinger =
        (static_cast<const char *>(cfg["server"]["OptLinger"])) == "true";
    int threadNum =
        std::stoi(static_cast<const char *>(cfg["server"]["threadNum"]));

    int sqlPort = std::stoi(static_cast<const char *>(cfg["mysql"]["port"]));
    const char *sqlUser = static_cast<const char *>(cfg["mysql"]["user"]);
    const char *sqlPwd = static_cast<const char *>(cfg["mysql"]["password"]);
    const char *dbName = static_cast<const char *>(cfg["mysql"]["database"]);
    int connPoolNum =
        std::stoi(static_cast<const char *>(cfg["mysql"]["connPoolNum"]));

    std::string open = static_cast<const char *>(cfg["log"]["open"]);
    bool openLog = open == "true";
    LogLevel logLevel = [&cfg]() -> LogLevel {
        std::string level = static_cast<const char *>(cfg["log"]["logLevel"]);
        if (level == "DEBUG")
            return LogLevel::DEBUG;
        if (level == "INFO")
            return LogLevel::INFO;
        if (level == "WARN")
            return LogLevel::WARN;
        if (level == "ERROR")
            return LogLevel::ERROR;
        return LogLevel::INFO;
    }();

    int logQueSize =
        std::stoi(static_cast<const char *>(cfg["log"]["logQueueSize"]));
    return std::make_tuple(port,
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
}

/**
 * @brief 启动web服务器
 * 
 */
void WebServer::Start()
{
    int timeMS = -1; // epoll wait timeout
    if (!isClose_)
    {
        LOG_INFO("========== Server start ==========");
    }
    while (!isClose_)
    {
        if (timeoutMS_ > 0)
        {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++)
        {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if (fd == listenFd_)
            {
                // 处理监听事件 接受连接
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 处理错误 关闭连接
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if (events & EPOLLIN)
            {
                // 处理读事件
                assert(users_.count(fd) > 0);
                OnRead_(&users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                // 处理写事件
                assert(users_.count(fd) > 0);
                OnWrite_(&users_[fd]);
            }
            else
            {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

/**
 * @brief 初始化网络套接字
 * 
 * @return true 
 * @return false 
 */
bool WebServer::InitSocket_()
{
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024)
    {
        LOG_ERROR("Port:%d error!", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = {0};
    if (openLinger_)
    {
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }
    ret = setsockopt(
        listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        close(listenFd_);
        LOG_ERROR("Init linger error!");
        return false;
    }
    int optval = 1;
    ret = setsockopt(listenFd_,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (const void *)&optval,
                     sizeof(int));
    if (ret == -1)
    {
        close(listenFd_);
        LOG_ERROR("Set reuse address error!");
        return false;
    }
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        close(listenFd_);
        LOG_ERROR("Bind Port:%d error!", port_);
        return false;
    }
    ret = listen(listenFd_, 6);
    if (ret < 0)
    {
        close(listenFd_);
        LOG_ERROR("Listen port:%d error!", port_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0)
    {
        close(listenFd_);
        LOG_ERROR("Add listen error!");
        return false;
    }
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

/**
 * @brief 初始化事件模式
 * 
 * @param trigMode 
 */
void WebServer::InitEventMode_(int trigMode)
{
    listenEvent_ = EPOLLRDHUP; // 被挂断事件
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP; // 保证多线程下只有一个线程处理一个连接
    switch (trigMode)
    {
    case 0:
        break;
    // LT + ET
    case 1:
        connEvent_ |= EPOLLET;
        break;
    // ET + LT
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    // ET + ET
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    // 默认：ET + ET
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ = EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

/**
 * @brief 添加一个客户端连接
 * 
 * @param fd 
 * @param addr 
 */
void WebServer::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if (timeoutMS_ > 0)
    {
        timer_->add(fd,
                    timeoutMS_,
                    std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    // 绑定客户端的读事件和触发模式
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    // 将文件描述符设置为非阻塞
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].getFd());
}

/**
 * @brief 接受连接
 * 
 */
void WebServer::DealListen_()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do
    {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if (fd <= 0)
        {
            return;
        }
        else if (HttpConn::userCount >= MAX_FD)
        {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

/**
 * @brief 处理写事件
 * 
 * @param client 
 */
void WebServer::DealWrite_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

/**
 * @brief 处理读事件
 * 
 * @param client 
 */
void WebServer::DealRead_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

/**
 * @brief 发送错误信息
 * 
 * @param fd 
 * @param info 
 */
void WebServer::SendError_(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
    {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

/**
 * @brief 延长客户端连接时间
 * 
 * @param client 
 */
void WebServer::ExtentTime_(HttpConn *client)
{
    assert(client);
    if (timeoutMS_ > 0)
    {
        timer_->adjust(client->getFd(), timeoutMS_);
    }
}

/**
 * @brief 关闭与客户端的连接
 * 
 * @param client 
 */
void WebServer::CloseConn_(HttpConn *client)
{
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    epoller_->DelFd(client->getFd());
    client->Close();
}

/**
 * @brief 处理读事件
 * 
 * @param client 
 */
void WebServer::OnRead_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    // 读失败且错误码不是EAGAIN 说明对端关闭连接
    if (ret <= 0 && readErrno != EAGAIN)
    {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

/**
 * @brief 处理写事件
 * 
 * @param client 
 */
void WebServer::OnWrite_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0)
    {
        // 传输完成
        if (client->isKeepAlive())
        {
            OnProcess(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)
        {
            // 继续传输
            epoller_->ModFd(client->getFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

/**
 * @brief 处理客户端请求
 * 
 * @param client 
 */
void WebServer::OnProcess(HttpConn *client)
{
    if (client->process())
    {
        /* 处理请求成功， 绑定写就绪事件 */
        epoller_->ModFd(client->getFd(), connEvent_ | EPOLLOUT);
    }
    else
    {
        /* 处理请求失败， 需要继续读取*/
        epoller_->ModFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}

/**
 * @brief 设置文件描述符为非阻塞
 * 
 * @param fd 
 * @return int 
 */
int WebServer::SetFdNonblock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
