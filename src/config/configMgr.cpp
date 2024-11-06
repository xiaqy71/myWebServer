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