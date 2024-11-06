/**
 * @file configMgr.cpp
 * @author xiaqy (792155443@qq.com)
 * @brief 配置管理类实现
 * @version 0.1
 * @date 2024-11-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "configMgr.h"

configMgr &configMgr::Instance()
{
    static configMgr inst;
    return inst;
}

bool configMgr::init(const char *file_path)
{
    return load(file_path);
}