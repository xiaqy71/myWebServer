# myWebServer

## 前言

本项目基于[WebServer](https://github.com/markparticle/WebServer)，在此项目做了些许改动与优化，并添加了注释

## 如何运行本项目

### 依赖

mysql

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

## 待解决

- 日志系统可以设置最大行数，超过最大行数时会创建新文件。如果某天已经写了两个文件，重新启动服务器程序时会在第一个日志文件后追加内容
