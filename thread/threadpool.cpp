/*************************************************************************
    > File Name: threadpool.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 08 Nov 2021 02:15:51 PM CST
 ************************************************************************/

#include "threadpool.h"
#include <utils/exception.h>
#include <log/log.h>

#define LOG_TAG "threadpool"
#define THREAD_NUM_ONCE 2

namespace Jarvis {

static thread_local Fiber *gMainFiber = nullptr;
thread_local std::function<void()> ThreadPool::mIdle;

ThreadPool::ThreadPool() :
    mMinThreadNum(4),
    mMaxThreadNum(8),
    mBusyNum(0),
    mAliveNum(0),
    mShouldStop(false),
    mManagerThread(nullptr)
{

}

ThreadPool::ThreadPool(uint32_t minThreadNum, uint32_t maxThreadNum) :
    mMinThreadNum(minThreadNum),
    mMaxThreadNum(maxThreadNum),
    mBusyNum(0),
    mAliveNum(0),
    mShouldStop(false),
    mManagerThread(nullptr)
{
    Init(minThreadNum, maxThreadNum);
}

ThreadPool::~ThreadPool()
{
   stop();
   msleep(10);  // 等待线程退出, 简单粗暴
}

bool ThreadPool::Init(uint32_t minThreadNum, uint32_t maxThreadNum)
{
    if (mManagerThread != nullptr) {
        return true;
    }

    this->mMinThreadNum = minThreadNum;
    this->mMaxThreadNum = maxThreadNum;

    if (minThreadNum == 0 || minThreadNum > maxThreadNum) {
        String8 msg = String8::format("ThreadPool Invalid Param (min %d, max %d)",
            minThreadNum, maxThreadNum);
        throw Exception(msg);
    }

    mManagerThread.reset(new Thread(std::bind(&ThreadPool::manager, this), "ThreadPool::Manager"));
    mWorkerThreads.resize(mMaxThreadNum);
    for (int i = 0; i < mMaxThreadNum; ++i) {
        mWorkerThreads[i].reset(new Thread(std::bind(&ThreadPool::worker, this), "ThreadPool::WorkerThread"));
    }
    return true;
}

bool ThreadPool::ShouldStop()
{
    return mShouldStop.load();
}

bool ThreadPool::start()
{
    if (mManagerThread->ThreadStatus() == Thread::THREAD_RUNNING) {
        return true;
    }
    if (mManagerThread == nullptr) {
        mManagerThread.reset(new Thread(std::bind(&ThreadPool::manager, this), "ThreadPool::Manager"));
        LOG_ASSERT(mManagerThread.get(), "");
    }
    mManagerThread->run();
    int successed = 0;
    for (int i = 0; i < mMaxThreadNum; ++i) {
        if (mWorkerThreads[i] == nullptr) {
            mWorkerThreads[i].reset(new Thread(std::bind(&ThreadPool::worker, this), "ThreadPool::WorkerThread"));
            continue;
        }
        if (successed < mMinThreadNum) {
            mWorkerThreads[i]->run();
            ++successed;
        }
    }
    mAliveNum = successed;

    for (const auto &it : mWorkerThreads) { // start 之后数组内不能存在空指针
        LOG_ASSERT(it != nullptr, "");
    }
    LOGI("Manager and Worker is running");
    return true;
}

void ThreadPool::stop()
{
    LOGD("%s()", __func__);
    mShouldStop = true;
    for (auto &it : mWorkerThreads) {
        it->Interrupt();
    }
    mManagerThread->Interrupt();

    for (int i = 0; i < mMaxThreadNum; ++i) {
        mPoolCond.signal();
    } // 唤醒卡在wait的线程
}

int ThreadPool::manager()
{
    LOGD("manager() start");
    while (ShouldStop() == false) {
        mPoolMutex.lock();
        uint32_t queueSize = QueueSize();
        int busyNum = mBusyNum.load();
        mPoolMutex.unlock();
        int aliveNum = mAliveNum.load();

        LOGD("%s() queueSize = %d, busyNum = %d, aliveNum = %d, mMinThreadNum = %d, mMaxThreadNum = %d", 
            __func__, queueSize, busyNum, aliveNum, mMinThreadNum, mMaxThreadNum);

        // 任务过多，添加线程
        if ((queueSize / 4 > aliveNum) && (aliveNum < mMaxThreadNum)) {
            for (int i = 0, count = 0; i < mMaxThreadNum && THREAD_NUM_ONCE > count; ++i) {
                if (mWorkerThreads[i]->ThreadStatus() != Thread::THREAD_RUNNING) {
                    mWorkerThreads[i]->reset(std::bind(&ThreadPool::worker, this));
                    mWorkerThreads[i]->run();
                    LOGI("%s() Thread %s %d is running", __func__, mWorkerThreads[i]->getName().c_str(),
                        mWorkerThreads[i]->getKernalTid());
                    {
                        AutoLock<Mutex> lock(mPoolMutex);
                        mAliveNum++;
                    }
                    ++count;
                }
            }
        }

        // 任务过少，减少线程
        if (queueSize < aliveNum && aliveNum > mMinThreadNum) {
            mExitNum = THREAD_NUM_ONCE;
            for (int i = 0; i < THREAD_NUM_ONCE; ++i) {
                mPoolCond.signal();
            }
        }

        msleep(1000);
    }

    // 线程退出
    LOGD("ThreadPool Manager Thread is exit");
    return Thread::THREAD_EXIT;
}

int ThreadPool::worker()
{
    LOGD("worker() start");
    gMainFiber = Fiber::GetThis().get(); // 每个线程的主协程
    //Fiber::sp idleFiber(new Fiber(mIdle));

    while (ShouldStop() == false) {
        while (QueueSize() == 0) {    // 队列为空时线程等待
            AutoLock<Mutex> lock(mPoolMutex);
            LOGD("Thread %ld is waiting", gettid());
            mPoolCond.wait(mPoolMutex);
            if (mExitNum.load() > 0) {
                mExitNum--;
                mAliveNum--;
                ThreadExit();
                return Thread::THREAD_WAITING;
            }
        }
        const ThreadPool::FiberOrCallBack &fc = front();
        if (fc.fiber == nullptr && fc.cb == nullptr) {
            LOGW("FiberOrCallBack is null");
            continue;
        }
        ++mBusyNum;
        if (fc.fiber) {
            fc.fiber->Resume();
        } else if (fc.cb) {
            fc.cb();
        }
        --mBusyNum;
    }

    return Thread::THREAD_EXIT;
}

ThreadPool::FiberOrCallBack ThreadPool::front()
{
    ThreadPool::FiberOrCallBack fc;
    AutoLock<Mutex> lock(mTaskMutex);
    auto it = mTaskQuesue.begin();
    fc = *it;
    mTaskQuesue.pop_front();
    return fc;
}

uint32_t ThreadPool::QueueSize()
{
    AutoLock<Mutex> lock(mTaskMutex);
    return mTaskQuesue.size();
}

void ThreadPool::ThreadExit()
{
    LOGD("Thread %ld exit", gettid());
}
}
