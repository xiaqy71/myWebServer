#include <gtest/gtest.h>
#include "log.h"

TEST(LogTest, Init)
{
    Log::Instance()->init(LogLevel::DEBUG, "./log", ".log", 1024);
    EXPECT_EQ(Log::Instance()->IsOpen(), true);
}