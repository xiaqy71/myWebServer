/**
 * @file configMgr.h
 * @author xiaqy (792155443@qq.com)
 * @brief 配置管理类声明
 * @version 0.1
 * @date 2024-11-06
 *
 * @copyright Copyright (c) 2024
 *
 */
#if !defined(CONFIGMGR_H)
#define CONFIGMGR_H

#include "xini_file.h"

class configMgr : public xini_file_t
{
public:
    configMgr(const configMgr &) = delete;
    configMgr &operator=(const configMgr &) = delete;

    static configMgr &Instance();
    bool init(const char *file_path);

private:
    configMgr() { }
    ~configMgr() { }
};

#endif // CONFIGMGR_H)
