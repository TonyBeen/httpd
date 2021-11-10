/*************************************************************************
    > File Name: epoll.cpp
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:35:37 PM CST
 ************************************************************************/

#include "epoll.h"
#include "config.h"
#include "http/http.h"
#include <utils/exception.h>
#include <log/log.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define LOG_TAG "epoll"

#define EPOLL_EVENT_SIZE 1024
#define READ_BUFSIZE     1024
#define WRITE_BUFSIZE    1024

namespace Jarvis {
static uint32_t gMinThreadNum;
static uint32_t gMaxThreadNum;
static String8  root;
static String8  gMysqlUser;
static String8  gMysqlPasswd;
static String8  gDatabaseName;

Epoll::Epoll(const Address &addr) :
    mEpollFd(0)
{
    LoadConfig();
    try {
        mWorkerThreadPool = new ThreadPool(gMinThreadNum, gMaxThreadNum);
        LOG_ASSERT(mWorkerThreadPool != nullptr, "%s %d %s()", __FILE__, __LINE__, __func__);
    } catch (const Exception &e) {
        LOGE("%s() %s", __func__, e.what());
        exit(0);
    }
    
    mServerSocket = new Socket(Socket::TCP, Socket::IPv4, addr.mPort, addr.mIp);
    LOG_ASSERT(mServerSocket != nullptr, "");

    if (Reinit()) {
        mWorkerThreadPool->start();
        mMysqlDb = new MySqlConn(gMysqlUser.c_str(), gMysqlPasswd.c_str(), gDatabaseName.c_str());
        LOG_ASSERT(mMysqlDb != nullptr, "%s %d %s()", __FILE__, __LINE__, __func__);
        LOGI("epoll is working");
    }
}

Epoll::Epoll(const Address::sp &addr) :
    Epoll(addr != nullptr ? *addr.get() : Address(getLocalAddress()[0] , 80))
{
}

Epoll::~Epoll()
{
    if (mEpollFd > 0) {
        close(mEpollFd);
    }
    if (mServerSocket != nullptr) {
        delete mServerSocket;
    }
    if (mWorkerThreadPool != nullptr) {
        delete mWorkerThreadPool;
    }
    if (mMysqlDb) {
        delete mMysqlDb;
    }
}

void Epoll::LoadConfig()
{
    gMinThreadNum = Config::Lookup<uint32_t>("threadpool.min_thread_num", 4);
    gMaxThreadNum = Config::Lookup<uint32_t>("threadpool.max_thread_num", 8);
    root = Config::Lookup<const char *>("html.root", "/home/hsz/VScode/www/html/");
    gMysqlUser = Config::Lookup<const char *>("mysql.user", "mysql");
    gMysqlPasswd = Config::Lookup<const char *>("mysql.password", "123456");
    gDatabaseName = Config::Lookup<const char *>("mysql.database", "userdb");
}

bool Epoll::Reinit()
{
    if (mEpollFd <= 0) {
        mEpollFd = epoll_create(EPOLL_EVENT_SIZE);
        if (mEpollFd <= 0) {
            LOGE("%s:%s() epoll_create error. errno = %d, str: %s",
                __FILE__, __func__, errno, strerror(errno));
            return false;
        }
    }

    if (mServerSocket->mValid) {
        mEvent.data.fd = mServerSocket->mSockFd;
        mEvent.events  = EPOLLIN | EPOLLET;
        int nRetCode = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mServerSocket->mSockFd, &mEvent);
        if (nRetCode && errno != EEXIST) {
            LOGE("%s:%s() epoll_ctl error. errno = %d, str: %s",
                __FILE__, __func__, errno, strerror(errno));
            return false;
        }
    } else {
        return false;
    }

    return true;
}

int Epoll::main_loop(void *arg)
{
    Epoll *epoll = static_cast<Epoll *>(arg);
    sockaddr_in clientAddr;
    socklen_t addrLen;
    if (epoll->mEpollFd <= 0) {
        LOG_ASSERT(epoll->Reinit(), "epoll fd is invalid");
    }
    epoll_event *eventAll = new epoll_event[EPOLL_EVENT_SIZE];
    LOG_ASSERT(eventAll, "new epoll_event failed");
    std::shared_ptr<epoll_event> eventTmp(eventAll, [](epoll_event *ptr) {  // 作用，当循环退出时不用在手动释放内存
        if (ptr) {
            delete []ptr;
        }
    });

    while (true) {
        int nRet = epoll_wait(epoll->mEpollFd, eventAll, EPOLL_EVENT_SIZE, -1);
        if (nRet < 0 && errno != EAGAIN) {
            LOGE("epoll_wait error. errno = %d, str: %s", errno, strerror(errno));
            break;
        }
        LOGI("epoll_wait events = %d", nRet);
        for (int i = 0; i < nRet; ++i) {
            epoll_event &event = eventAll[i];
            int cfd = event.data.fd;
            if (event.data.fd == epoll->mServerSocket->mSockFd) {  // accept;
                epoll->AcceptEvent();
            }
            if (event.events & (EPOLLHUP | EPOLLRDHUP)) {  // 对端关闭连接
                LOGI("client %d shut down", event.data.fd);
                epoll_ctl(epoll->mEpollFd, EPOLL_CTL_DEL, event.data.fd, nullptr);
                epoll->mServerSocket->close(event.data.fd);
            }
            if ((event.events & EPOLLIN) && (cfd != mServerSocket->mSockFd)) {   // 读事件
                LOGD("read event addWork");
                epoll->mWorkerThreadPool->addWork(std::bind(&Epoll::ReadEventProcess, epoll, cfd));
            }
            if (event.events & EPOLLOUT) {  // 写事件
                LOGI("%s:%d %s() EPOLLOUT", __FILE__, __LINE__, __func__);
            }
        }
    }
    return Thread::THREAD_WAITING;
}

void Epoll::ReadEventProcess(int fd)
{
    LOGI("%s()", __func__);
    LOG_ASSERT(fd > 0, "%s:%d %s()", __FILE__, __LINE__, __func__);
    if (mServerSocket->mClientFdMap.find(fd) == mServerSocket->mClientFdMap.end()) {
        LOGD("%s() client fd %d is shut down", __func__, fd);
        return; // fd已关闭
    }
    char readBuf[READ_BUFSIZE] = {0};
    int readSize = READ_BUFSIZE;
    ByteBuffer buffer;
    while (true) {
        readSize = ::read(fd, readBuf, READ_BUFSIZE);
        if (readSize <= 0) {
            if (errno != EAGAIN && errno != 0) {
                LOGE("%s() read socket %d return %d. errcode %d, %s", 
                    __func__, fd, readSize, errno, strerror(errno));
                return;
            }
            break;
        }
        buffer.append(readBuf, readSize);
    }
    if (buffer.size() == 0) {
        return;
    }
    LOGI("buffer: \n%s", buffer.const_data());
    HttpParser parser(buffer);
    HttpMethod method = parser.getMethod();
    HttpVersion version = parser.getVersion();
    String8 url = parser.getUrl();
    static const String8 &defaultHtml = "index.html";

    HttpResponse response;
    std::map<String8, String8> body;

    body.insert(std::make_pair("Server", HttpResponse::GetDefaultResponseByKey("Server")));
    body.insert(std::make_pair("Connection", HttpResponse::GetDefaultResponseByKey("Connection")));

    switch (method) {
    case HttpMethod::GET:
        if (url == "/login") {
            ProcessLogin(url, parser);
        }

        if (url == "/") {
            body.insert(std::make_pair("Content-Type", "text/html; charset=utf-8"));
            body.insert(std::make_pair("Content-Length", String8::format("%d", GetFileLength(root + defaultHtml))));

            String8 resHeader = response.CreateHttpReponseHeader(version, HttpStatus::OK);
            String8 resBody = response.CreateHttpReponseBody(body);
            SendToClient(fd, resHeader + resBody, root + defaultHtml);
            break;
        }
        {
            int32_t dotIdx = url.find_last_of(".");
            if (dotIdx < 0) {
                Send404(fd);
                break;
            }
            String8 fileExten = String8(url.c_str() + dotIdx + 1);  // 文件扩展名
            body.insert(std::make_pair("Content-Type", String8::format("%s", GetContentTypeByFileExten(fileExten).c_str())));
            body.insert(std::make_pair("Content-Length", String8::format("%d", GetFileLength(root + url))));
            String8 resHeader = response.CreateHttpReponseHeader(version, HttpStatus::OK);
            String8 resBody = response.CreateHttpReponseBody(body);
            SendToClient(fd, resHeader + resBody, root + url);
        }

        break;
    case HttpMethod::POST:
        break;
    default:
        break;
    }
    LOGD("%s() end", __func__);
}

bool Epoll::ProcessLogin(String8 &url, const HttpParser &parser)
{
    const HttpParser::HttpRquestMap &reqMap = parser.getRequestMap(); 
    if (reqMap.size() == 0) {
        url = "/";
        return false;
    } 

    auto it = reqMap.find("username");
    if (it == reqMap.end()) {
        return false;
    }
    String8 username = it->second;
    it = reqMap.find("password");
    if (it == reqMap.end()) {
        return false;
    }
    String8 password = it->second;
    
}

void Epoll::SendToClient(int fd, const String8 &responseHeader, const String8 &filePath)
{
    LOGD("response \n%s", responseHeader.c_str());
    LOGD("filePath: %s", filePath.c_str());

    struct stat st = {0};
    if (stat(filePath.c_str(), &st) != 0) {
        LOGE("%s() stat %s error %d, errstr: %s", __func__, filePath.c_str(), errno, strerror(errno));
        Send404(fd);
        return;
    }

    ssize_t ret = 0;
    ret = ::send(fd, responseHeader.c_str(), responseHeader.length(), 0);
    if (ret <= 0) {
        if (errno == EAGAIN) {
            ::send(fd, responseHeader.c_str(), responseHeader.length(), 0);
        } else {
            LOGE("send error %d, errstr: %s", errno, strerror(errno));
            return;
        }
    }
    size_t fileLength = st.st_size;
    int fileDes = open(filePath.c_str(), O_RDONLY);
    char buf[READ_BUFSIZE];
    int readedSize = 0;
    while (true) {
        memset(buf, 0, READ_BUFSIZE);
        readedSize = ::read(fileDes, buf, READ_BUFSIZE);
        if (readedSize <= 0) {
            if (errno != EAGAIN) {
                LOGE("%s() read %s error %d, errstr: %s", __func__, filePath.c_str(), errno, strerror(errno));
            }
            break;
        }
        ret = ::send(fd, buf, readedSize, 0);
        if (ret < 0) {
            LOGE("send to socket[%d] error %d, errstr: %s", fd, errno, strerror(errno));
            break;
        }
    }
    LOGD("%s() end", __func__);
}

void Epoll::Send404(int fd)
{
    LOGI("%s()", __func__);
    static const char *defaultHeader = {
        "HTTP/1.0 404 Not Found\r\n Server: Jarvis V1.0\r\n Content-Type: text/html; charset=utf-8\r\n" };
    struct stat st;
    static const String8 html404 = String8(root + "/404.html");
    int ret = stat(html404.c_str(), &st);
    if (ret < 0) {
        return;
    }
    uint32_t fileLength = st.st_size;
    String8 header = String8(defaultHeader) + String8::format("Content-Length: %d\r\n\r\n", fileLength);
    ret = ::send(fd, header.c_str(), header.length(), 0);

    int fileDes = open(html404.c_str(), O_RDONLY);
    char buf[READ_BUFSIZE];
    int readedSize = 0;
    while (true) {
        memset(buf, 0, READ_BUFSIZE);
        readedSize = ::read(fileDes, buf, READ_BUFSIZE);
        if (readedSize <= 0) {
            if (errno != EAGAIN) {
                LOGE("%s() read error %d, errstr: %s", __func__, errno, strerror(errno));
            }
            break;
        }
        ret = ::send(fd, buf, readedSize, 0);
        if (ret < 0) {
            LOGE("send error %d, errstr: %s", errno, strerror(errno));
            break;
        }
    }
}

void Epoll::AcceptEvent()
{
    LOGI("%s()", __func__);
    static uint8_t accept_once = 1;
    uint8_t i = 0;
    while (i < accept_once) {      // 持续接收
        if (mServerSocket->mClientFdMap.size() == EPOLL_EVENT_SIZE - 1) {
            LOGE("epoll capacity has reached its limit");
            return;
        }
        sockaddr_in clientAddr;
        int clientSock = mServerSocket->accept(&clientAddr);
        if (clientSock > 0) {
            LOGD("%s() accept client %d: [%s:%d]", __func__, clientSock,
                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            epoll_event eventNew;
            int flags = fcntl(clientSock, F_GETFL, 0);
            fcntl(clientSock, F_SETFL, flags | O_NONBLOCK);
            eventNew.data.fd = clientSock;
            eventNew.events = EPOLLIN | EPOLLET;
            epoll_ctl(mEpollFd, EPOLL_CTL_ADD, clientSock, &eventNew);
        } else if (clientSock <= 0) {
            LOGW("accept error: errno %d, errstr: %s", errno, strerror(errno));
            break;
        }
        ++i;
    }
}

} // namespace Jarvis
