/**
 * @file httprequest.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "httprequest.h"

/**
 * @brief 默认的html文件
 * 
 */
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

/**
 * @brief 默认的html文件标签
 * 
 */
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

/**
 * @brief 初始化
 * 
 */
void HttpRequest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

/**
 * @brief 解析请求
 * 
 * @param buff 
 * @return true 
 * @return false 
 */
bool HttpRequest::parse(Buffer &buff)
{
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0)
    {
        return false;
    }
    while (buff.ReadableBytes() && state_ != FINISH)
    {
        const char *lineEnd =
            std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd);
        // 利用状态机解析请求
        switch (state_)
        {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line))
            {
                return false;
            }
            ParsePath_();
            break;
        case HEADERS:
            ParseHeader_(line);
            /* 如果可读字节数小于2， 说明没有内容实体， 解析完成*/
            if (buff.ReadableBytes() <= 2)
            {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if (lineEnd == buff.BeginWriteConst())
            break;
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG(
        "[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

std::string HttpRequest::path() const { return path_; }

std::string &HttpRequest::path() { return path_; }

std::string HttpRequest::method() const { return method_; }

std::string HttpRequest::version() const { return version_; }

/**
 * @brief 获取post请求
 * 
 * @param key 
 * @return std::string 
 */
std::string HttpRequest::GetPost(const std::string &key) const
{
    return GetPost(key.c_str());
}

std::string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

/**
 * @brief 是否保持连接
 * 
 * @return true 
 * @return false 
 */
bool HttpRequest::IsKeepAlive() const
{
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection")->second == "keep-alive" &&
               version_ == "1.1";
    }
    return false;
}

/**
 * @brief 解析请求行
 * 
 * @param line 
 * @return true 
 * @return false 
 */
bool HttpRequest::ParseRequestLine_(const std::string &line)
{
    // 正则表达式解析请求行
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten))
    {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

/**
 * @brief 解析请求首部字段[首部字段为可选项]
 * 
 * @param line 
 */
void HttpRequest::ParseHeader_(const std::string &line)
{
    /*
        正则表达式匹配首部字段
        格式为 key: value
    */
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten))
    {
        header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        /*遇到空行， 开始解析内容实体*/
        state_ = BODY;
    }
}

/**
 * @brief 解析body
 * 
 * @param line 
 */
void HttpRequest::ParseBody_(const std::string &line)
{
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

/**
 * @brief 解析文件路径
 * 
 */
void HttpRequest::ParsePath_()
{
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        /*如果请求路径在预设路径中， 为其加上.html后缀*/
        for (auto &it : DEFAULT_HTML)
        {
            if (it == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

/**
 * @brief 解析post请求
 * 
 */
void HttpRequest::ParsePost_()
{
    if (method_ == "POST" &&
        header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_))
        {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1)
            {
                /* 判断当前请求是否为登录请求*/
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin))
                {
                    path_ = "/welcome.html";
                }
                else
                {
                    path_ = "/error.html";
                }
            }
        }
    }
}

/**
 * @brief 解析url编码
 * 
 */
void HttpRequest::ParseFromUrlencoded_()
{
    if (body_.size() == 0)
    {
        return;
    }
    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;
    for (; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (!post_.count(key) && j < i)
    {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

/**
 * @brief 用户验证 处理登录和注册请求
 * 
 * @param name 
 * @param pwd 
 * @param isLogin 
 * @return true 
 * @return false 
 */
bool HttpRequest::UserVerify(const std::string &name,
                             const std::string &pwd,
                             bool isLogin)
{
    if (name == "" || pwd == "")
        return false;
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin)
    {
        flag = true;
    }
    /* 查询用户及密码 */
    snprintf(order,
             256,
             "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
             name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order))
    {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        /* 登录行为 */
        if (isLogin)
        {
            if (pwd == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        }
        else
        {
            /* 注册行为 则用户名已被使用 */
            flag = false;
            LOG_DEBUG("user used!");
        }
    }

    mysql_free_result(res);
    /* 注册行为 且 用户名未被使用*/
    if (!isLogin && flag == true)
    {
        memset(order, 0, 256);
        snprintf(order,
                 256,
                 "INSERT INTO user(username, password) VALUES('%s', '%s')",
                 name.c_str(),
                 pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    LOG_DEBUG("UserVerify success!");
    return flag;
}

/**
 * @brief 转换16进制
 * 
 * @param ch 
 * @return int 
 */
int HttpRequest::ConverHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}
