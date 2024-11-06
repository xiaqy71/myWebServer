#if !defined(CONFIGMGR_H)
#define CONFIGMGR_H

#include "xini_file.h"

class configMgr: public xini_file_t
{
public:
    configMgr(const configMgr &) = delete;
    configMgr &operator=(const configMgr &) = delete;

    static configMgr& Instance();
    bool init(const char* file_path);
private:
    configMgr() {}
    ~configMgr() {}
};

#endif // CONFIGMGR_H)
