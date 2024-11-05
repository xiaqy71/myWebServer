# myWebServer

## 前言

本项目基于[WebServer](https://github.com/markparticle/WebServer)，在此项目做了些许改动与优化，并添加了注释

## 改进

1. 依据《Effective C++》中的准则，将公共逻辑抽取到一个私有函数中，从而重用代码并减少重复。

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

2. 使用现代C++优化部分代码

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

使用 `std::call_once`确保 `Log`实例只被创建一次，并且是线程安全的

```cpp
Log *Log::Instance()
{
    static Log *inst = nullptr;
    std::call_once(initInstanceFlag_, []() {
        inst = new Log();
    });
    return inst;
}
```

## 待解决

1. 日志系统可以设置最大行数，超过最大行数时会创建新文件。如果某天已经写了两个文件，重新启动服务器程序时会在第一个日志文件后追加内容
