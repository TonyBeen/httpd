/*************************************************************************
    > File Name: thread.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 08 Nov 2021 02:42:48 PM CST
 ************************************************************************/

#include "thread.h"
#include <log/log.h>
#include <utils/Errors.h>

#define LOG_TAG "thread"

namespace Jarvis {

Thread::Thread() :
    mThreadName(""),
    mThreadFunc(nullptr),
    mThreadId(0),
    mKernalTid(0),
    mIsDetach(true),
    mThreadStatus(THREAD_EXIT),
    callBackReturn(THREAD_NO_RETURN)
{

}

Thread::Thread(std::function<int()> cb, const String8& threadName, bool isDetach) :
    mThreadFunc(cb),
    mThreadName(threadName),
    mIsDetach(isDetach),
    mThreadId(0),
    mKernalTid(0),
    mThreadStatus(THREAD_EXIT),
    callBackReturn(THREAD_NO_RETURN)
{

}

Thread::~Thread()
{
    mShouldExit.store(true);
    if (!mIsDetach) {
        join();
    }
}

void Thread::setDetach(bool f)
{
    if (mThreadStatus.load() == THREAD_EXIT) {
        mIsDetach = f;
    }
}

void Thread::reset(std::function<int()> cb)
{
    AutoLock<Mutex> lock(mMutex);
    mThreadFunc = cb;
}

int Thread::join()
{
    if (!mIsDetach) {
        pthread_join(mThreadId, nullptr);
    }
    return callBackReturn;
}

void Thread::Interrupt()
{
    mShouldExit.store(true);
    if (mThreadStatus.load() == THREAD_WAITING) {
        mCond.broadcast();
    }
}

int Thread::run(uint32_t stackSize)
{
    int ret = 0;
    switch (mThreadStatus)
    {
    case THREAD_EXIT:
        ret = start(stackSize);
        break;
    case THREAD_WAITING:
        mCond.broadcast();
    default:
        LOGD("unknow status");
        break;
    }
    return ret;
}

int Thread::start(uint32_t stackSize)
{
    if (mThreadStatus.load() != THREAD_EXIT) {
        return INVALID_OPERATION;
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackSize > 0) {
        pthread_attr_setstacksize(&attr, stackSize);
        // char *stack = (char *)malloc(stackSize);
        // if (stack != nullptr) {
        //     pthread_attr_setstack(&attr, stack, stackSize);
        // }
    }
    if (mIsDetach) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    int ret = pthread_create(&mThreadId, &attr, threadloop, this);
    if (ret == 0) {
        mThreadStatus = THREAD_RUNNING;
    } else {
        LOGE("%s() pthread_create error %d, %s", __func__, errno, strerror(errno));
    }
    return ret;
}

void *Thread::threadloop(void *arg)
{
    LOG_ASSERT(arg, "never be null");
    Thread *thread = static_cast<Thread *>(arg);
    thread->mKernalTid = gettid();
    
    while (thread->ShouldExit() == false) {
        {
            AutoLock<Mutex> lock(thread->mMutex);   // 防止执行时期切换函数
            if (thread->mThreadFunc != nullptr) {
                thread->callBackReturn = thread->mThreadFunc();
            }
        }
        
        if (thread->callBackReturn == THREAD_EXIT || thread->ShouldExit()) {
            thread->mThreadStatus = THREAD_EXIT;
            break;
        }

        AutoLock<Mutex> lock(thread->mMutex);
        thread->mThreadStatus = THREAD_WAITING;
        thread->mCond.wait(thread->mMutex);
    }
    return nullptr;
}

} // namespace Jarvis
