/*************************************************************************
    > File Name: epoll.h
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:35:29 PM CST
 ************************************************************************/

#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "socket.h"
#include "address.h"
#include "fiber.h"
#include "thread/threadpool.h"
#include "http/http_parser.h"
#include <sqlutils/mysql.h>
#include <sys/epoll.h>
#include <vector>
#include <memory>
#include <map>

namespace Jarvis {
class Epoll
{
    DISALLOW_COPY_AND_ASSIGN(Epoll);
public:
    Epoll(const Address &addr);
    Epoll(const Address::sp &addr);
    virtual ~Epoll();

    size_t getClientNum() const { return mServerSocket->mClientFdMap.size(); }
    virtual int main_loop(void *arg);

protected:
    virtual bool Reinit();

    virtual void ReadEventProcess(int fd);
    virtual bool ProcessLogin(String8 &url, const HttpParser &parser);
    virtual void SendToClient(int fd, const String8 &responseHeader, const String8 &filePath);
    virtual void Send404(int fd);
    virtual void AcceptEvent();
    virtual void LoadConfig();

private:
    int                 mEpollFd;
    Socket *            mServerSocket;
    epoll_event         mEvent;
    ThreadPool *        mWorkerThreadPool;  // 处理http请求工作线程
    MySqlConn *         mMysqlDb;
};


} // namespace Jarvis

#endif // __EPOLL_H__