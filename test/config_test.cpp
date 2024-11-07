#include <gtest/gtest.h>
#include "configMgr.h"

TEST(CONFIG_TEST, open_file) {
    bool ret = configMgr::Instance().init("config.ini");
    EXPECT_EQ(ret, true);
}

TEST(CONFIG_TEST, read_config) {
    bool ret = configMgr::Instance().init("config.ini");
    EXPECT_EQ(ret, true);
    auto& cfg = configMgr::Instance();
    std::string host = static_cast<const char*>(cfg["mysql"]["port"]);
    EXPECT_EQ(host, "3306");
}