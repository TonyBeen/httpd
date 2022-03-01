/*************************************************************************
    > File Name: threadpool.h
    > Author: hsz
    > Brief:
    > Created Time: Mon 08 Nov 2021 02:15:36 PM CST
 ************************************************************************/

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "fiber.h"
#include "thread.h"
#include <utils/string8.h>
#include <utils/Errors.h>
#include <utils/mutex.h>
#include <memory>
#include <functional>
#include <list>
#include <atomic>
#include <vector>

namespace eular {

class ThreadPool
{
public:
    typedef std::shared_ptr<ThreadPool> sp;
    ThreadPool();
    ThreadPool(uint32_t minThreadNum, uint32_t maxThreadNum);
    virtual ~ThreadPool();

    bool Init(uint32_t minThreadNum, uint32_t maxThreadNum);
    bool start();

    template<typename FiberOrCb>
    void addWork(FiberOrCb fc)
    {
        AutoLock<Mutex> lock(mTaskMutex);
        mTaskQuesue.push_back(FiberOrCallBack(fc));
        mPoolCond.signal();
    }

    /**
     * @brief 设置线程空闲时间执行函数
     */
    void setIdle(std::function<void()> v) { mIdle = v; }

protected:
    bool ShouldStop();

    struct FiberOrCallBack {
        Fiber::SP fiber;
        std::function<void()> cb;

        FiberOrCallBack() : fiber(nullptr), cb(nullptr) { }
        FiberOrCallBack(Fiber::SP p) : fiber(p), cb(nullptr) { }
        FiberOrCallBack(std::function<void()> p) : fiber(nullptr), cb(p) { }
        FiberOrCallBack(const FiberOrCallBack& fc)
        {
            fiber = fc.fiber;
            cb = fc.cb;
        }
        FiberOrCallBack &operator=(const FiberOrCallBack& fc)
        {
            fiber = fc.fiber;
            cb = fc.cb;
            return *this;
        }
        void reset() {
            fiber = nullptr;
            cb = nullptr;
        }
    };

    int     manager();
    int     worker();
    void    stop();

    FiberOrCallBack front();
    uint32_t    QueueSize();
    void        ThreadExit();

private:
    uint32_t                    mMinThreadNum;
    uint32_t                    mMaxThreadNum;
    std::atomic<uint32_t>       mBusyNum;
    std::atomic<uint32_t>       mAliveNum;
    std::atomic<uint8_t>        mExitNum;
    std::atomic<bool>           mShouldStop;

    std::vector<Thread::sp>     mWorkerThreads;
    Thread::sp                  mManagerThread;
    mutable Mutex               mThreadPoolMutex;
    Mutex                       mPoolMutex;
    std::list<FiberOrCallBack>  mTaskQuesue;
    Mutex                       mTaskMutex;
    Condition                   mPoolCond;

    static thread_local std::function<void()> mIdle;      // 线程空闲时执行函数
};

} // namespace eular

#endif // __THREAD_POOL_H__