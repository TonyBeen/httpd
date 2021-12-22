/*************************************************************************
    > File Name: thread.h
    > Author: hsz
    > Brief: utils/thread 为C风格线程，此处为引入functional的C++风格线程
    > Created Time: Mon 08 Nov 2021 02:42:45 PM CST
 ************************************************************************/

#ifndef __HTTPD_THREAD_H__
#define __HTTPD_THREAD_H__

#include <utils/utils.h>
#include <utils/condition.h>
#include <utils/mutex.h>
#include <utils/string8.h>
#include <functional>
#include <memory>
#include <atomic>

#define THREAD_NO_RETURN (0xFFF)

namespace eular {
class Thread
{
public:
    DISALLOW_COPY_AND_ASSIGN(Thread);

    typedef std::shared_ptr<Thread> sp;
    Thread();
    Thread(std::function<int()>, const String8& threadName = "", bool isDetach = true);
    virtual ~Thread();
    enum ThreadStatus {
        THREAD_EXIT = 0,
        THREAD_RUNNING = 1,
        THREAD_WAITING = 2
    };

    uint8_t         ThreadStatus() const { return mThreadStatus.load(); }
    void            setDetach(bool f = true);
    void            setName(const String8& name) { mThreadName = name; }
    const String8&  getName() const { return mThreadName; }
    uint32_t        getKernalTid() const { return mKernalTid; }
    int             getFuncRet() const { return callBackReturn; }

    void            reset(std::function<int()> cb);
    int             join();
    void            Interrupt();
    int             run(uint32_t stackSize = 0);

protected:
    static void *threadloop(void *);
    bool ShouldExit() const { return mShouldExit.load(); }
    int             start(uint32_t stackSize);

private:
    mutable Mutex           mMutex;
    Condition               mCond;
    String8                 mThreadName;
    std::atomic<bool>       mShouldExit;
    uint32_t                mKernalTid;
    pthread_t               mThreadId;

    bool                    mIsDetach;
    std::function<int()>    mThreadFunc;
    std::atomic<uint8_t>    mThreadStatus;
    int  callBackReturn;
};


} // namespace eular

#endif // __HTTPD_THREAD_H__
