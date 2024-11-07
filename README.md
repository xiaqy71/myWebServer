# myWebServer

## 前言

本项目基于[WebServer](https://github.com/markparticle/WebServer)，在此项目做了些许改动与优化，并添加了注释

## 如何运行本项目

### 依赖

- cmake 3.20版本及以上
- mysql

使用以下命令获取MySQL C API的包含目录和库目录

```bash
mysql_config --cflags
mysql_config --libs
```

修改项目根目录下 `CMakeLists.txt`中的链接库目录

```cmake
link_directories(/usr/lib64/mysql)
```

MySQL配置方法，见[WebServer](https://github.com/markparticle/WebServer)

### 运行

```bash
git clone https://github.com/xiaqy71/myWebServer.git
cd myWebServer
cd build
cmake ..
make
./Server
```

> [!TIP]
> 运行前需要修改config.ini中的配置(修改build目录中的config.ini无需重新构建，修改源码中的config.ini则需要重新构建)

config.ini示例配置如下

```ini
[server]
port = 8080
trigMode = 3
timeoutMS = 60000
OptLinger =  false # true or false
threadNum = 6
[mysql]
port = 3306
user = xiaqy
password = 123456
database = WebServer
connPoolNum = 12
[log]
open = true # true or false
LogLevel = DEBUG # DEBUG or WARN or INFO 
logQueueSize = 1024
```

### 测试

```bash
cd build
ctest
```

## 改进

- 依据《Effective C++》中的准则，将公共逻辑抽取到一个私有函数中，从而重用代码并减少重复。

```cpp
const char *Buffer::BeginWriteConst() const
{
    return const_cast<Buffer*>(this)->BeginWrite();
}

char *Buffer::BeginWrite()
{
    return BeginPtr_() + writePos_;
}
```

- 使用现代C++优化部分代码

例如，原代码

```cpp
unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
deque_ = move(newDeque);
  
std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
writeThread_ = move(NewThread);
```

改为使用 `std::make_unique`提升效率

```cpp
deque_ = std::make_unique<BlockDeque<std::string>>();
writeThread_ = std::make_unique<std::thread>(FlushLogThread);
```

- 使用GoogleTest对代码进行测试

```cmake
# 测试
cmake_policy(SET CMP0135 NEW)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)
```

- 使用configMgr类，读取配置，通过配置文件修改程序配置，避免每次修改配置都需要重新编译的麻烦
- 增强封装性，将 SqlConnPool类中获取MySQL连接和还回MySQL连接的方法设为私有，将 `SqlConnRAII`设为其友元类，只允许通过RAII思想使用sql连接，避免内存泄露
- 原代码根据执行目录获取资源目录，这样执行命令只能在固定目录进行，改为获取程序所在目录，因此只要将资源目录和程序放在同一目录，无论在哪里执行程序，都可以正常获取资源文件。

原代码如下

```cpp
srcDir_ = getcwd(nullptr, 256);
assert(srcDir_);
strncat(srcDir_, "/resources/", 16);
```

修改后代码如下

```cpp
char exePath[256] = {0};
ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
exePath[len] = '\0';
auto dirPath =
    std::string(exePath).substr(0, std::string(exePath).find_last_of('/')) + "/resources/";
srcDir_ = new char[dirPath.size() + 1];
memcpy(srcDir_, dirPath.c_str(), dirPath.size() + 1);
```

## 待解决

- 日志系统可以设置最大行数，超过最大行数时会创建新文件。如果某天已经写了两个文件，重新启动服务器程序时会在第一个日志文件后追加内容
