/*************************************************************************
    > File Name: test/test_threadpool.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 09 Nov 2021 04:03:43 PM CST
 ************************************************************************/

#include "thread/threadpool.h"
#include <log/log.h>
#include <assert.h>
#include <iostream>

#define LOG_TAG "test_threadloop"

using namespace std;
using namespace Jarvis;

void func(int i)
{
    msleep(500);
    LOGD("func(arg = %d)", i);
    msleep(500);
    LOGD("thread %ld execute over", gettid());
}

void thread_addWork(ThreadPool *th)
{
    int count = 0;
    while (count < 20) {
        th->addWork(std::bind(func, 0));
        msleep(200);
        ++count;
    }
    sleep(5);
}

int main(int argc, char **argv)
{
    InitLog();
    addOutputNode(LogWrite::FILEOUT);
    ThreadPool th;
    assert(th.Init(2, 4));
    for (int i = 0; i < 10; ++i) {
        th.addWork(std::bind(func, i));
    }
    assert(th.start());

    thread_addWork(&th);
    LOGD("main exit");
    return 0;
}
