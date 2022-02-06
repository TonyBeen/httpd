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
#include "curl.h"
#include "thread/threadpool.h"
#include "http/http_parser.h"
#include "http/http_response.h"
#include "sql/mysql.h"
#include <utils/mutex.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <vector>
#include <memory>
#include <map>

namespace eular {

struct LoginInfo {
    int             fd;
    String8         userName;
    String8         loginIP;
    time_t          loginTime;

    LoginInfo &operator=(const LoginInfo &info)
    {
        this->fd        =  info.fd;
        this->userName  =  info.userName;
        this->loginIP   =  info.loginIP;
        this->loginTime =  info.loginTime;
    }
};

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
    virtual void SendResponse(const HttpResponse &httpRes);
    virtual void SendFile(int fd, const String8 &responseHeader, const String8 &filePath);
    virtual void SendJson(int fd, const String8 &header, const String8 &json);
    virtual void Send404(int fd);
    virtual void Send400(int fd);
    virtual void LoadConfig();
    virtual int  ReadHttpHeader(int fd, ByteBuffer &buf);

private:
    int                 mEpollFd;
    epoll_event         mEvent;
    ThreadPool *        mWorkerThreadPool;  // 处理http请求工作线程 unused
    MySqlConn *         mMysqlDb;
    Mutex               mEpollMutex;
    Curl                mLocateAddressAPI;
    std::map<int, sockaddr_in> mClientMap;
};


} // namespace eular

#endif // __EPOLL_H__