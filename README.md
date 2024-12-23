
# myWebServer

## 概述

myWebServer 是基于 [WebServer](https://github.com/markparticle/WebServer) 项目进行改进与优化的版本，加入了更详细的注释，提升了部分代码的性能与结构。

## 项目亮点

- 高并发模型：采用 IO 多路复用（epoll）和线程池，实现多线程的 Reactor 高并发模型，提升服务器的并发处理能力。

- HTTP 请求解析：使用正则表达式和有限状态机解析 HTTP 请求报文，支持静态资源请求，并具备良好的可扩展性。

- 自动扩容缓冲区：基于标准库容器封装，实现了自动增长的缓冲区，支持高效的数据读写操作，避免缓冲区溢出。

- 高效的定时器管理：采用小根堆实现定时器，及时关闭超时的非活动连接，有效管理连接资源，提高服务器性能。

- 异步日志系统：基于单例模式和阻塞队列构建异步日志系统，支持日志分级、按文件大小和日期生成，方便运行状态的记录与追踪。

- 数据库连接池：实现线程安全的 MySQL 数据库连接池，支持连接复用，减少连接建立和关闭的开销，实现了用户注册和登录功能。

- 配置文件支持：新增配置文件 config.ini，支持服务器参数的灵活配置，无需重新编译即可调整端口号、触发模式等参数，提升了维护性。

- 单元测试：引入 Google Test 对关键模块进行单元测试，确保代码的可靠性和稳定性。

- 代码优化：对原项目的代码结构进行重构和优化，增加了详细的注释，提升了代码的可读性和可维护性。

## 依赖环境

- CMake 3.20 及以上版本
- MySQL

### MySQL 依赖配置

1. 获取 MySQL C API 的包含目录和库目录：
    ```bash
    mysql_config --cflags
    mysql_config --libs
    ```
2. 修改项目根目录下的 `CMakeLists.txt`，设置链接库目录：
    ```cmake
    link_directories(/usr/lib64/mysql)
    ```

> 详细 MySQL 配置方法，请参考 [WebServer 项目](https://github.com/markparticle/WebServer)。

## 如何运行

### 步骤

1. 克隆项目并构建：
    ```bash
    git clone https://github.com/xiaqy71/myWebServer.git
    cd myWebServer
    mkdir build && cd build
    cmake ..
    make
    ```

2. 启动服务器：
    ```bash
    ./Server
    ```

> [!TIP]
> 在运行服务器之前，需先修改 `config.ini` 文件中的配置。如果修改的是 `build` 目录下的 `config.ini`，无需重新构建；若修改源码中的 `config.ini`，则需要重新构建项目。

### 示例配置 (`config.ini`)

```ini
[server]
port = 8080
trigMode = 3
timeoutMS = 60000
OptLinger = false # true or false
threadNum = 6

[mysql]
port = 3306
user = root
password = rootpassword
database = testdb
connPoolNum = 12

[log]
open = true # true or false
LogLevel = DEBUG # DEBUG or WARN or INFO
logQueueSize = 1024
```

## 测试

运行单元测试：

```bash
cd build
ctest
```

## 项目改进

- **代码重用与简化**：依据《Effective C++》中的准则，将公共逻辑抽取为私有函数，减少重复代码。例如：
    ```cpp
    const char *Buffer::BeginWriteConst() const {
        return const_cast<Buffer*>(this)->BeginWrite();
    }

    char *Buffer::BeginWrite() {
        return BeginPtr_() + writePos_;
    }
    ```

- **使用现代 C++ 特性**：例如用 `std::make_unique` 替换旧的智能指针初始化方式，提高代码效率：
    ```cpp
    deque_ = std::make_unique<BlockDeque<std::string>>();
    writeThread_ = std::make_unique<std::thread>(FlushLogThread);
    ```

- **单元测试框架**：使用 GoogleTest 进行代码测试，测试配置示例如下：
    ```cmake
    # test gtest
    add_executable(hello_test test/hello_test.cpp)
    target_link_libraries(hello_test GTest::gtest_main)
    gtest_discover_tests(hello_test)
    ```

- **增强封装性**：`SqlConnPool` 类中获取和还回 MySQL 连接的方法设为私有，并将 `SqlConnRAII` 设为其友元类，利用 RAII 原则管理连接，防止内存泄漏。

- **路径获取优化**：原项目基于执行目录获取资源文件路径，限制了执行命令的目录。改进后，程序获取自身路径，确保资源文件与程序同目录时可在任意位置执行。例如：
    ```cpp
    char exePath[256] = {0};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    exePath[len] = '\0';
    auto dirPath =
        std::string(exePath).substr(0, std::string(exePath).find_last_of('/')) +
        "/resources/";
    srcDir_ = new char[dirPath.size() + 1];
    ```

- **配置改进**: 现在，服务器的配置参数存储在`config.ini`文件中，避免了硬编码配置。您可以在不重新编译程序的情况下， 更改端口号、触发模式等参数。

## 待解决问题

- **日志文件重启追加问题**：日志系统当前支持设置最大行数，超过时创建新文件。但如果服务器重新启动，日志会追加到第一个文件中，而不是从最新的日志文件继续记录。

## 致谢

感谢以下项目和资料对本项目的支持和贡献：

- [Google Test](https://github.com/google/googletest): 本项目使用的单元测试框架。

- [inifile](https://github.com/Gaaagaa/inifile): 感谢作者提供的配置文件解析实现。

- [WebServer](https://github.com/markparticle/WebServer): 本项目参考的原始项目，提供了基础和灵感。

- 《高性能Linux服务器编程》 -游双著：本项目的主要参考书籍，提供了宝贵的知识和指导。

## 开源协议

本项目基于MIT许可证开源，您可以自由使用、修改和分发本软件。
