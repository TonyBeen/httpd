/*************************************************************************
    > File Name: epoll.cpp
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:35:37 PM CST
 ************************************************************************/

#include "epoll.h"
#include "config.h"
#include "http/http.h"
#include "util/json.h"
#include <utils/exception.h>
#include <log/log.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <list>

#define LOG_TAG "epoll"

#define EPOLL_EVENT_SIZE 2048
#define READ_BUFSIZE     1024
#define WRITE_BUFSIZE    1024

namespace eular {
static uint32_t gMinThreadNum;
static uint32_t gMaxThreadNum;
static String8  root;
static String8  gMysqlUser;
static String8  gMysqlPasswd;
static String8  gDatabaseName;
static const String8 &gIndexHtml = "index.html";
static thread_local std::list<LoginInfo>    gUserLoginQueue;

Epoll::Epoll() :
    mEpollFd(0)
{
    LoadConfig();

    mLocateAddressAPI.setUrl("https://67ip.cn/check");
    mLocateAddressAPI.storeHeader("Host", "67ip.cn");
    mLocateAddressAPI.storeHeader("Connection", "keep-alive");
    mLocateAddressAPI.storeHeader("Accept", "application/json; */*");
    mLocateAddressAPI.storeHeader("User-Agent", "eular/httpd v1.0");
    mLocateAddressAPI.storeHeader("Cache-Control", "no-cache");

    mEpollMutex.setMutexName("epoll mutex");
    if (Reinit()) {
        // mWorkerThreadPool->start();
        mMysqlDb = new MySqlConn(gMysqlUser.c_str(), gMysqlPasswd.c_str(), gDatabaseName.c_str());
        LOG_ASSERT(mMysqlDb != nullptr, "%s %d %s()", __FILE__, __LINE__, __func__);
        LOGI("epoll is working");
    }
}

Epoll::~Epoll()
{
    ::close(mEpollFd); // epoll_wait状态直接关闭会导致什么问题?
    for (auto &it : mClientMap) {
        ::close(it.first);
    }
    delete mMysqlDb;
}

bool Epoll::addEvent(int fd, const sockaddr_in &addr)
{
    AutoLock<Mutex> lock(mEpollMutex);
    const auto &it = mClientMap.find(fd);
    if (it != mClientMap.end()) {
        LOGW("%s() socket fd %d already exists", __FUNCTION__, fd);
    }

    mClientMap[fd] = addr;
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    int keepAlive = 1;      // 开启keepalive属性
    int keepIdle = 60;      // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepInterval = 5;   // 探测时发包的时间间隔为5秒
    int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE,&keepIdle, sizeof(keepIdle));
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLET | EPOLLIN;
    int ret = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        if (errno == EEXIST) {
            epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &ev);
        } else {
            LOGE("%s() epoll_ctl error. errno %d, %s", __func__, errno, strerror(errno));
            return false;
        }
    }

    return true;
}

void Epoll::delEvent(int fd)
{
    AutoLock<Mutex> lock(mEpollMutex);
    mClientMap.erase(fd);
    epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr);
}

void Epoll::LoadConfig()
{
    // gMinThreadNum = Config::Lookup<uint32_t>("threadpool.min_thread_num", 4);
    // gMaxThreadNum = Config::Lookup<uint32_t>("threadpool.max_thread_num", 8);
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

    return true;
}

int Epoll::main_loop()
{
    sockaddr_in clientAddr;
    socklen_t addrLen;
    if (mEpollFd <= 0) {
        LOG_ASSERT(Reinit(), "epoll fd is invalid");
        mMysqlDb = new MySqlConn(gMysqlUser.c_str(), gMysqlPasswd.c_str(), gDatabaseName.c_str());
        LOG_ASSERT(mMysqlDb != nullptr, "%s %d %s()", __FILE__, __LINE__, __func__);
    }
    epoll_event *eventAll = new epoll_event[EPOLL_EVENT_SIZE];
    LOG_ASSERT(eventAll, "new epoll_event failed");
    std::shared_ptr<epoll_event> eventTmp(eventAll, [](const epoll_event *ptr) {  // 当循环退出时不用在手动释放内存
        if (ptr) {
            delete []ptr;
        }
    });

    while (true) {
        int nRet = epoll_wait(mEpollFd, eventAll, EPOLL_EVENT_SIZE, 5000);
        if (nRet < 0 && errno != EAGAIN) {
            LOGE("epoll_wait error. errno = %d, str: %s", errno, strerror(errno));
            break;
        }

        if (nRet == 0) {
            if (mLocateAddressAPI.isValid() == false) {
                mLocateAddressAPI.setUrl("https://67ip.cn/check");
            }

            for (const auto &it : gUserLoginQueue) {
                static const char *requestFmt = "ip=%s&token=a6fa55815ce40d6b1c7b4c5519298516";
                String8 request = String8::format(requestFmt, it.loginIP.c_str());
                mLocateAddressAPI.setFileds(request);
                if (mLocateAddressAPI.perform()) {
                    // TODO 解析http响应，根据状态码来处理数据
                    String8 response = mLocateAddressAPI.getResponse();
                    LOGD("api response: \n%s", response.c_str());
                    HttpResponseParser responseParser(response);
                    if (responseParser.getStatus() == HttpStatus::OK) {
                        JsonParser jp;
                        jp.Parse(responseParser.getResponseData().c_str());
                        int ret = jp.GetIntValByKey("code");
                        if (ret == 200) {
                            LOGD("country: %s, province: %s, city: %s: service: %s",
                                jp.GetStringValByKey("data.country").c_str(),
                                jp.GetStringValByKey("data.province").c_str(),
                                jp.GetStringValByKey("data.city").c_str(),
                                jp.GetStringValByKey("data.service").c_str());
                        }
                    }
                }
            }
            gUserLoginQueue.clear();
            continue;
        }

        LOGD("epoll_wait events = %d", nRet);
        for (int i = 0; i < nRet; ++i) {
            const epoll_event &event = eventAll[i];
            int cfd = event.data.fd;

            if (event.events & (EPOLLHUP | EPOLLRDHUP)) {  // 对端关闭连接
                const auto &it = mClientMap.find(cfd);
                LOG_ASSERT(it != mClientMap.end(), "");
                LOGI("main_loop() EPOLLHUP | EPOLLRDHUP event. client %d [%s:%u] shutdown", cfd, inet_ntoa(it->second.sin_addr), ntohs(it->second.sin_port));
                delEvent(cfd);
                ::close(cfd);
                continue;
            }
            if (event.events & EPOLLIN) {   // 读事件
                ReadEventProcess(cfd);
            }
            if (event.events & EPOLLOUT) {  // 写事件
                LOGI("%s:%d %s() EPOLLOUT write event", __FILE__, __LINE__, __func__);
            }
        }
    }
    return Thread::THREAD_WAITING;
}

void Epoll::ReadEventProcess(int fd)
{
    LOG_ASSERT(fd > 0, "%s:%d %s()", __FILE__, __LINE__, __func__);

    uint8_t readBuf[READ_BUFSIZE] = {0};
    int readSize = READ_BUFSIZE;
    ByteBuffer buffer;
    while (true) {
        readSize = ::read(fd, readBuf, READ_BUFSIZE);
        if (readSize < 0) {
            if (errno != EAGAIN && errno != 0) {
                LOGE("%s() read socket %d return %d. errcode %d, %s", 
                    __func__, fd, readSize, errno, strerror(errno));
                return;
            }
            break;
        } else if (readSize == 0) {
            auto it = mClientMap.find(fd);
            if (it == mClientMap.end()) {
                return;
            }
            LOGI("ReadEventProcess() close client %d, [%s:%u]", fd, inet_ntoa(it->second.sin_addr), ntohs(it->second.sin_port));
            delEvent(fd);
            ::close(fd);
            return;
        }
        buffer.append(readBuf, readSize);
        memset(readBuf, 0, READ_BUFSIZE);
    }
    if (buffer.size() == 0) {
        return;
    }
    LOGI("buffer: \n%s", buffer.const_data());
    HttpRequestParser parser(buffer);
    HttpMethod method = parser.getMethod();
    HttpVersion version = parser.getVersion();
    String8 url = parser.getUrl();

    String8 filePath;       // 要发送的文件及路径
    HttpResponse response(fd);

    response.setHttpVersion(version);
    response.addContent("Server", HttpResponse::GetDefaultResponseByKey("Server"));
    if (parser.KeepAlive()) {
        response.addContent("Connection", "keep-alive");
    } else {
        response.addContent("Connection", "close");
    }

    switch (method) {
    case HttpMethod::GET:
        if (url == "/") {
            response.setFilePath(root + gIndexHtml);
            response.setHttpStatus(HttpStatus::OK);
            SendResponse(response);
            auto it = mClientMap.find(fd);
            LOG_ASSERT(it != mClientMap.end(), "fd must exist in client map");
            LoginInfo info;
            info.fd = fd;
            info.loginIP = inet_ntoa(it->second.sin_addr);
            gUserLoginQueue.push_back(info);
            break;
        }

        if (url == "/register") {

        }

        {
            int32_t dotIdx = url.find('.');
            if (dotIdx < 0) {
                Send404(fd);
                break;
            }

            response.setFilePath(root + url);
            response.setHttpStatus(HttpStatus::OK);
            SendResponse(response);
        }

        break;
    case HttpMethod::POST:
        if (url == "/login") {
            ProcessLogin(url, parser, response);
            SendResponse(response);
            break;
        }

        break;
    default:
        Send400(fd);
        break;
    }

    if (!parser.KeepAlive()) {
        delEvent(fd);
    }
}

bool Epoll::ProcessLogin(String8 &url, const HttpRequestParser &parser, HttpResponse &response)
{
    const HttpRequestParser::HttpRquestMap &reqMap = parser.getRequestMap();
    auto it = reqMap.find("username");
    String8 username, password, cond;
    int ret;
    MySqlRes::sp res;

    if (reqMap.size() == 0 || it == reqMap.end()) {
        goto end;
    }
    username = it->second;
    it = reqMap.find("password");
    if (it == reqMap.end()) {
        goto end;
    }
    password = it->second;

    cond = String8::format("user_name = '%s' and user_password = '%s'", username.c_str(), password.c_str());
    ret = mMysqlDb->SelectSql("userinfo", "user_name, user_id, user_password", cond.c_str());
    if (ret != OK) {
        LOGE("%s() select sql error. cond: [%s] %d %s", __func__,
            cond.c_str(), mMysqlDb->getErrno(), mMysqlDb->getErrorStr());
        goto end;
    }

    res = mMysqlDb->getSqlRes();
    if (res->getDataCount() == 1) { // 防止sql注入
        // 登录成功
        LOGD("user name: %s login", username.c_str());
        // 获取用户登录信息
        LoginInfo info;
        info.fd = response.getClientSocket();
        auto it = mClientMap.find(info.fd);
        LOG_ASSERT(it != mClientMap.end(), "fd must exist in client map");
        info.userName = username;
        info.loginIP = inet_ntoa(it->second.sin_addr);
        time(&info.loginTime);
        gUserLoginQueue.push_back(info);

        try {
            JsonGenerator json;
            json.AddNode("code", 2);
            json.AddNode("message", "/home.html");
            response.setJson(json);
            response.setHttpStatus(HttpStatus::OK);
        } catch (const Exception &e) {
            LOGE("%s() catch throw. %s", __func__, e.what());
            //response.setFilePath(root + "home.html");
            response.setHttpStatus(HttpStatus::INTERNAL_SERVER_ERROR);
        }
        return true;
    }

end:
    if (parser.getRequestMap().find("if-modified-since") != parser.getRequestMap().end()) {
        response.setHttpStatus(HttpStatus::NOT_MODIFIED);
        static struct timespec t = {0, 0};
        if (t.tv_sec == 0) {
            clock_gettime(CLOCK_REALTIME, &t);
        }
        static const String8 &time = String8::format("%ld", t.tv_sec);
        response.addContent("Last-Modified", time.c_str());
        response.addContent("Cache-Control", "public, max-age=0");
        response.lock();
    } else {
        response.setHttpStatus(HttpStatus::OK);
        response.setFilePath(root + gIndexHtml);
    }
    
    return false;
}

void Epoll::SendFile(int fd, const String8 &responseHeader, const String8 &filePath)
{
    LOGD("response: \n%s", responseHeader.c_str());
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

void Epoll::SendJson(int fd, const String8 &header, const String8 &json)
{
    LOGD("response: \n%s", header.c_str());
    LOGD("json: \n%s", json.c_str());

    int ret = ::send(fd, header.c_str(), header.length(), 0);
    if (ret <= 0) {
        if (errno == EAGAIN) {
            if (::send(fd, header.c_str(), header.length(), 0) <= 0) {
                return;
            }
        } else {
            LOGE("send error %d, errstr: %s", errno, strerror(errno));
            return;
        }
    }

    ret = ::send(fd, json.c_str(), json.length(), 0);
    if (ret <= 0) {
        if (errno == EAGAIN) {
            ::send(fd, json.c_str(), json.length(), 0);
        } else {
            LOGE("send error %d, errstr: %s", errno, strerror(errno));
        }
    }
}

void Epoll::SendResponse(const HttpResponse &httpRes)
{
    int clientSock = httpRes.getClientSocket();
    String8 httpHdr = httpRes.CreateHttpReponseHeader() + httpRes.CreateHttpReponseBody();

    HttpResponse::ResponseType type = httpRes.getResponseType();

    switch (type) {
    case HttpResponse::ResponseType::FILE:
        SendFile(clientSock, httpHdr, httpRes.getFilePath());
        break;
    case HttpResponse::ResponseType::JSON:
        SendJson(clientSock, httpHdr, httpRes.getJson());
        break;
    case HttpResponse::ResponseType::XML:
        break;
    case HttpResponse::ResponseType::YAML:
        break;
    default:
        break;
    }
}

void Epoll::Send404(int fd)
{
    LOGD("%s()", __func__);
    static const char *defaultHeader = {
        "HTTP/1.0 404 Not Found\r\n Server: eular/httpd v1.0\r\n Content-Type: text/html; charset=utf-8\r\n" };
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
        if (readedSize == 0) {
            break;
        }
        if (readedSize < 0) {
            LOGE("%s() read error [%d,%s]", __func__, errno, strerror(errno));
            break;
        }
        ret = ::send(fd, buf, readedSize, 0);
        if (ret < 0) {
            LOGE("send error. [%d,%s]", errno, strerror(errno));
            break;
        }
    }
}

void Epoll::Send400(int fd)
{
    static const char *defaultHeader = {
        "HTTP/1.0 400 Bad Request\r\n"
        "Server: eular/httpd v1.0\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: 0"};
    ::send(fd, defaultHeader, strlen(defaultHeader), 0);
}

int Epoll::ReadHttpHeader(int fd, ByteBuffer &buf)
{
    int ret = 0;
    char tmp[READ_BUFSIZE] = {0};

    while (1) {
        ret = recv(fd, tmp, READ_BUFSIZE, MSG_PEEK);
        if (ret <= 0) {
            if (errno != EAGAIN && errno != 0) {
                LOGE("%s() read socket %d return %d. errcode %d, %s", __func__, fd, ret, errno, strerror(errno));
                return 0;
            }
            break;
        }
        int32_t index = String8::kmp_strstr(tmp, "\r\n\r\n");
        if (index < 0) {
            buf.append((uint8_t *)tmp, ret);
            ret = recv(fd, tmp, READ_BUFSIZE, 0);
        } else {
            memset(tmp, 0, READ_BUFSIZE);
            ret = recv(fd, tmp, index + 4, 0);
            buf.append((uint8_t *)tmp, ret);
            break;
        }
        memset(tmp, 0, READ_BUFSIZE);
    }

    return buf.size();
}

} // namespace eular
