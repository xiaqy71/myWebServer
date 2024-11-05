# myWebServer

## 前言

本项目基于[WebServer](https://github.com/markparticle/WebServer)，在此项目做了些许改动与优化

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
