/*************************************************************************
    > File Name: test_thread.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 08 Nov 2021 04:22:23 PM CST
 ************************************************************************/

#include "thread/thread.h"

using namespace eular;
using namespace std;

int test_func(int fd)
{
    printf("%s(%d) tid = %ld\n", __func__, fd, gettid());
    return Thread::THREAD_EXIT;
}

int test_func2(int fd)
{
    printf("%s(%d) tid = %ld\n", __func__, fd, gettid());
    return Thread::THREAD_WAITING;
}

int main(int argc, char **argv)
{
    int fd = 3;
    Thread th(std::bind(test_func2, fd), "Thread-test", false);
    th.run();
    sleep(1);
    printf("th.tid = %u\n", th.getKernalTid());

    th.reset(std::bind(test_func, fd));
    th.run();
    sleep(1);
    printf("th.tid = %u\n", th.getKernalTid());

    th.join();

    return 0;
}
