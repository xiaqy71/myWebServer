#include <gtest/gtest.h>
#include "threadpool.h"

TEST(ThreadPool_TEST, Init)
{
    ThreadPool pool(4);
    int num = 0;
    for (int i = 0; i < 10; i++)
    {
        pool.AddTask([&num, i] {
            ++ num;
            GTEST_LOG_(INFO) << "task" << i << std::endl;
        });
    }
    sleep(1);
    EXPECT_EQ(num, 10);
}