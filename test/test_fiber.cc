#include "fiber.h"
#include <log/log.h>
#include <utils/thread.h>

#define LOG_TAG "test_fiber"
using namespace eular;

void test_fiber()
{
    LOGD("test_fiber begin");
    LOGD("test_fiber stop");
    Fiber::Yeild2Hold();
    LOGD("test_fiber begin");
    LOGD("test_fiber stop");
    Fiber::Yeild2Hold();
    LOGD("test_fiber end");
}

int thread_func(void *)
{
    LOGD("thread %ld start", gettid());
    Fiber::sp ptr = Fiber::GetThis();
    Fiber::sp fiber(new Fiber(test_fiber));
    LOG_ASSERT(fiber, "");
    LOGD("resume");
    fiber->Resume();
    LOGD("thread after swapin");

    fiber->Resume();
    LOGD("thread after swapin 2");
    fiber->Resume();
    LOGD("thread stop");
    LOGD("refer count %d\n", fiber.use_count());

    return Thread::THREAD_EXIT;
}

int main(int argc, char **argv)
{
    LOGD("main start");
    Thread th("test_fiber", thread_func);
    th.setArg(nullptr);
    th.run();

    sleep(2);

    LOGD("main exit");
    return 0;
}