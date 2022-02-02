/*************************************************************************
    > File Name: socket.cpp
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:45:59 PM CST
 ************************************************************************/

#include "socket.h"
#include "config.h"
#include "util/json.h"
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/exception.h>
#include <fcntl.h>
#include <atomic>
#include <netdb.h>
#include <list>

#define LOG_TAG "socket"
#define LISTEN_SOCKET_NUM 512

namespace eular {
static uint32_t     gSizeOfEpollVec  = 0;
std::list<String8>  gLoginUserInfo;         // 登录用户信息

static std::atomic<uint32_t> gCountUdpFd = {3};
Socket::Socket() // : Socket(TCP, IPv4, 80, "")
{
    mValid = false;
}

Socket::Socket(ProtocolType protocol, SockFamily family, uint16_t port, const String8 &ip) :
    mProtocolType(protocol),
    mFamily(family),
    mPort(port),
    mIPAddr(ip),
    mValid(false)
{
    InitSocket();
    switch (mProtocolType) {
    case TCP:
        CreateTCP();
        break;
    case UDP:
        CreateTCP();
        break;
    default:
        break;
    }
}

Socket::~Socket()
{
    destroy();
}

int Socket::ResetSocket()
{
    destroy();
    InitSocket();
    switch (mProtocolType) {
    case TCP:
        CreateTCP();
        break;
    case UDP:
        CreateTCP();
        break;
    default:
        break;
    }
}

int Socket::accept(sockaddr_in *addr)
{
    int clientFd = -1;
    socklen_t addrLen;
    if (mProtocolType == TCP) {
        clientFd = ::accept(mSockFd, (sockaddr *)addr, &addrLen);
        if (clientFd > 0 && addr) {
            mClientFdMap[clientFd] = *addr;
        } else {
            LOGE("accept error. errno %d, errstr %s", errno, strerror(errno));
        }
    } else if (mProtocolType == UDP) {
        char buf[128] = {0};
        // NOTE: client needs to send data
        ssize_t readSize = ::recvfrom(mSockFd, buf, 128, 0, (sockaddr *)addr, &addrLen);
        if (readSize > 0) {
            clientFd = ++gCountUdpFd;
            mClientFdMap[gCountUdpFd.load()] = *addr;
        }
    }

    return clientFd;
}

int Socket::close(int fd)
{
    auto it = mClientFdMap.find(fd);
    if (it == mClientFdMap.end()) {
        return INVALID_PARAM;
    }
    mClientFdMap.erase(it);
    ::close(fd);

    return 0;
}

ssize_t Socket::recv(int sock, char *buf, uint32_t bufSize)
{
    auto it = mClientFdMap.find(sock);
    if (it == mClientFdMap.end()) {
        LOGE("socket fd %d is non-existent", sock);
        return INVALID_PARAM;
    }
    bzero(buf, bufSize);
    ssize_t readSize = UNKNOWN_ERROR;
    switch (mProtocolType) {
    case TCP:
        readSize = ::read(sock, buf, bufSize);
        if (readSize <=0 && errno != EAGAIN) {
            LOGE("read %s, %d, %s", readSize, errno, strerror(errno));
            mClientFdMap.erase(it);
        }
        break;
    case UDP:
        sockaddr_in clientAddr;
        socklen_t addrLen;
        readSize = ::recvfrom(mSockFd, buf, bufSize, 0, (sockaddr *)&(clientAddr), &addrLen);
        if (readSize > 0) {
            mClientFdMap[sock] = clientAddr;
        }
        break;
    default:
        LOGE("Unknow protocol type! %d", mProtocolType);
    }
    return readSize;
}

ssize_t Socket::send(int sock, const char *buf, uint32_t size)
{
    auto it = mClientFdMap.find(sock);
    if (it == mClientFdMap.end()) {
        LOGE("socket fd %d is non-existent", sock);
        return INVALID_PARAM;
    }
    ssize_t writeSize = UNKNOWN_ERROR;
    switch (mProtocolType)
    {
    case TCP:
        writeSize = ::write(sock, buf, size);
        break;
    case UDP:
        writeSize = ::sendto(mSockFd, buf, size, 0, (sockaddr *)&(it->second), sizeof(it->second));
        break;
    default:
        break;
    }
    return writeSize;
}

int Socket::setnonblock()
{
    int flags = fcntl(mSockFd, F_GETFL, 0);
    return ::fcntl(mSockFd, F_SETFL, flags | O_NONBLOCK);
}

int Socket::getsockopt(int level, int optname, void *optval, socklen_t *optlen)
{
    return ::getsockopt(mSockFd, level, optname, optval, optlen);
}

int Socket::setsockopt(int level, int optname, const void *optval, socklen_t optlen)
{
    return ::setsockopt(mSockFd, level, optname, optval, optlen);
}

void Socket::InitSocket()
{
    mSockFd = ::socket(mFamily, mProtocolType, 0);
    if (mSockFd < 0) {
        String8 msg = String8::format("socket error. errno = %d, error string: %s", errno, strerror(errno));
        throw Exception(msg);
    }
}

void Socket::CreateUDP()
{
    if (mSockFd > 0) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        bzero(&addr, len);
        if (mIPAddr.length() > 0) {
            addr.sin_addr.s_addr = inet_addr(mIPAddr.c_str());
        } else {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        addr.sin_family = mFamily;
        addr.sin_port = htons(mPort);
        int flag = 1;
        ::setsockopt(mSockFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
        if (::bind(mSockFd, (sockaddr *)&addr, len)) {
            String8 msg = String8::format("bind error. code %d, str: %s", errno, strerror(errno));
            throw Exception(msg);
        }
        mValid = true;
    }
}

void Socket::CreateTCP()
{
    if (mSockFd > 0) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        bzero(&addr, len);
        if (mIPAddr.length() > 0) {
            addr.sin_addr.s_addr = inet_addr(mIPAddr.c_str());
        } else {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        addr.sin_family = mFamily;
        addr.sin_port = htons(mPort);
        int flag = 1;
        ::setsockopt(mSockFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
        if (::bind(mSockFd, (sockaddr *)&addr, len)) {
            String8 msg = String8::format("bind error. code %d, str: %s", errno, strerror(errno));
            throw Exception(msg);
        }
        if (::listen(mSockFd, LISTEN_SOCKET_NUM)) {
            String8 msg = String8::format("listen error. code %d, str: %s", errno, strerror(errno));
            throw Exception(msg);
        }
        mValid = true;
    }
}

sockaddr_in Socket::getClientAddr(uint32_t clientSock)
{
    sockaddr_in addr;
    auto it = mClientFdMap.find(clientSock);
    if (it == mClientFdMap.end()) {
        return addr;
    }
    return it->second;
}

void Socket::destroy()
{
    if (mSockFd > 0) {
        close(mSockFd);
    }

    if (mProtocolType == TCP) {
        for (auto it = mClientFdMap.begin(); it != mClientFdMap.end(); ++it) {
            if (it->first > 0) {
                close(it->first);
            }
        }
    }
    
    mClientFdMap.clear();
    mValid = false;
}

TcpServer::TcpServer(uint16_t port, const String8 &IP) :
    Socket(TCP, IPv4, port, IP)
{
    gSizeOfEpollVec = Config::Lookup<uint32_t>("epollvec.size", 5);

    for (int i = 0; i < gSizeOfEpollVec; ++i) {
        std::shared_ptr<Epoll> epollTmp(new Epoll());
        LOG_ASSERT(epollTmp != nullptr, "");
        std::shared_ptr<Thread> threadTmp(new Thread(std::bind(&Epoll::main_loop, epollTmp.get()), "epoll thread"));
        LOG_ASSERT(threadTmp != nullptr, "");
        mEpollProcMap[threadTmp] = epollTmp;
        threadTmp->run();
    }
}

TcpServer::~TcpServer()
{

}

int TcpServer::accept(sockaddr_in *addr)
{
    socklen_t addrLen;
    sockaddr_in tmp;

    int clientFd = ::accept(mSockFd, (sockaddr *)&tmp, &addrLen);
    if (clientFd > 0) {
        // add to epoll
        Epoll *epoll = mEpollProcMap.begin()->second.get();
        int min = mEpollProcMap.begin()->second->getClientCount();
        for (auto &it : mEpollProcMap) {
            if (min > it.second->getClientCount()) {
                min = it.second->getClientCount();
                epoll = it.second.get();
            }
        }

        LOGI("accept client %d, [%s:%u]", clientFd, inet_ntoa(tmp.sin_addr), ntohs(tmp.sin_port));
        LOG_ASSERT(epoll, "");
        if (tmp.sin_addr.s_addr == 0 || tmp.sin_port == 0) {
            close(clientFd);
            return 0;
        }
        epoll->addEvent(clientFd, tmp);

        static String8 IPmsg;
        static String8 apiIP;
        String8 acceptIP = inet_ntoa(tmp.sin_addr);
        // https://67ip.cn/check?ip=39.102.104.241&token=a6fa55815ce40d6b1c7b4c5519298516
        // https://www.36ip.cn/?ip=39.106.218.123
#if 0
        hostent *host = gethostbyname("67ip.cn");
        if (host != nullptr) {
            char *tmp = host->h_addr_list[0];
            apiIP = inet_ntoa(*(struct in_addr*)tmp);
            try {
                TcpClient tcpClient(80, apiIP);
                if (tcpClient.connect() == 0) {
                    static const char *requestBufFmt =
                        "GET /check?ip=%s&token=a6fa55815ce40d6b1c7b4c5519298516 HTTP/1.1\r\n"
                        "Host: 67ip.cn\r\n"
                        "Connection: keep-alive\r\n"
                        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36\r\n"
                        "Pragma: no-cache\r\n"
                        "X-Forwarded-For: 111.165.64.99\r\n"
                        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n"
                        "Cache-Control: no-cache\r\n\r\n";
                    char buf[1024] = {0};
                    char recvBuf[4096] = {0};
                    int n = snprintf(buf, sizeof(buf), requestBufFmt, acceptIP.c_str());
                    LOGD("send buf:\n%s", buf);
                    if (tcpClient.send((const uint8_t *)buf, n) > 0) {
                        n = tcpClient.recv((uint8_t *)recvBuf, sizeof(recvBuf));
                        if (n > 0) {
                            LOGD("recv from %s: %s", apiIP.c_str(), recvBuf);
                            JsonParser jp;
                            jp.Parse(recvBuf, true);
                            int ret = jp.GetIntValByKey("code");
                            if (ret == 200) {
                                LOGD("country: %s, province: %s, city: %s: service: %s",
                                    jp.GetStringValByKey("data.country").c_str(),
                                    jp.GetStringValByKey("data.province").c_str(),
                                    jp.GetStringValByKey("data.city").c_str(),
                                    jp.GetStringValByKey("data.service").c_str());
                            }
                        }
                    } else {
                        LOGD("send %s failed", apiIP.c_str());
                    }
                } else {
                    LOGD("connect %s failed", apiIP.c_str());
                }
            } catch (const Exception &e) {
                LOGW("what() %s", e.what());
            }
        }
#endif
    } else {
        LOGE("accept error. errno %d, errstr %s", errno, strerror(errno));
    }

    if (addr) *addr = tmp;
    return clientFd;
}

// TODO: 第一次请求会接收一个全为0的IP
int TcpServer::accept_loop()
{
    epoll_event ev;
    int epollfd = epoll_create(2);
    if (epollfd < 0) {
        LOGE("%s() epoll_create error. %d: %s", __FUNCTION__, errno, strerror(errno));
        return Thread::THREAD_EXIT;
    }
    ev.data.fd = mSockFd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, mSockFd, &ev);
    epoll_event events[2];

    while (true) {
        int nRet = epoll_wait(epollfd, events, 2, 5000);
        if (nRet == 0) {
            continue;
        }
        if (nRet > 0) {
            for (int i = 0; i < nRet; ++i) {
                auto &event = events[i];
                if (event.data.fd == mSockFd) {
                    int fd = this->accept(nullptr);
                }
            }
        } else if (errno != EAGAIN) {
            LOGE("%s() epoll_wait error. %d: %s", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }

    close(epollfd);
    LOGD("%s() end", __func__);
    return Thread::THREAD_WAITING;
}

} // namespace eular
