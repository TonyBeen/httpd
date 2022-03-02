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
    Fiber::SP ptr = Fiber::GetThis();
    Fiber::SP fiber(new Fiber(test_fiber));
    LOG_ASSERT(fiber, "");
    LOGD("resume");
    fiber->resume();
    LOGD("thread after swapin");

    fiber->resume();
    LOGD("thread after swapin 2");
    fiber->resume();
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