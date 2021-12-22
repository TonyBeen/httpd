/*************************************************************************
    > File Name: epoll.h
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:35:29 PM CST
 ************************************************************************/

#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "address.h"
#include "fiber.h"
#include "thread/threadpool.h"
#include "http/http_parser.h"
#include "http/http_response.h"
#include <utils/mutex.h>
#include <sqlutils/mysql.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <vector>
#include <memory>
#include <map>

namespace eular {
class Epoll
{
    DISALLOW_COPY_AND_ASSIGN(Epoll);
public:
    Epoll();
    virtual ~Epoll();

    bool addEvent(int fd, const sockaddr_in &addr);
    void delEvent(int fd);
    size_t getClientCount() const { return mClientMap.size(); }
    virtual int main_loop();

protected:
    virtual bool Reinit();

    virtual void ReadEventProcess(int fd);
    virtual bool ProcessLogin(String8 &url, const HttpParser &parser, HttpResponse &response);
    virtual void SendToClient(const HttpResponse &httpRes);
    virtual void SendToClient(int fd, const String8 &responseHeader, const String8 &filePath);
    virtual void Send404(int fd);
    virtual void LoadConfig();
    virtual int  ReadHttpHeader(int fd, ByteBuffer &buf);

private:
    int                 mEpollFd;
    epoll_event         mEvent;
    ThreadPool *        mWorkerThreadPool;  // 处理http请求工作线程 unused
    MySqlConn *         mMysqlDb;
    Mutex               mEpollMutex;
    std::map<int, sockaddr_in> mClientMap;
};


} // namespace eular

#endif // __EPOLL_H__