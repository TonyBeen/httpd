/*************************************************************************
    > File Name: socket.cpp
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:45:59 PM CST
 ************************************************************************/

#include "socket.h"
#include "config.h"
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/exception.h>
#include <fcntl.h>
#include <atomic>

#define LOG_TAG "socket"
#define LISTEN_SOCKET_NUM 512

namespace eular {

static uint32_t gSizeOfEpollVec  = 0;

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
        epoll->addEvent(clientFd, tmp);
    } else {
        LOGE("accept error. errno %d, errstr %s", errno, strerror(errno));
    }

    if (addr) *addr = tmp;
    return clientFd;
}

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
        int nRet = epoll_wait(epollfd, events, 2, -1);
        if (nRet > 0) {
            for (int i = 0; i < nRet; ++i) {
                auto &event = events[i];
                if (event.data.fd = mSockFd) {
                    int fd = this->accept(nullptr);
                }
            }
        } else {
            if (errno != EAGAIN) {
                LOGE("%s() epoll_wait error. %d: %s", __FUNCTION__, errno, strerror(errno));
                break;
            }
        }
    }

    close(epollfd);
    LOGD("%s() end", __func__);
    return Thread::THREAD_WAITING;
}

TcpClient::TcpClient(uint16_t destPort, const String8& destAddr) :
    ClientBase(destPort, destAddr)
{
    mSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mSockFd < 0) {
        throw Exception(String8::format("socket error. [%d,%s]", errno, strerror(errno)));
    }
}

TcpClient::~TcpClient()
{
    destroy();
}

int TcpClient::connect()
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(mDestPort);
    addr.sin_addr.s_addr = inet_addr(mDestAddr.c_str());

    return ::connect(mSockFd, (struct sockaddr *)&addr, sizeof(addr));
}

int TcpClient::reset(uint16_t destPort, const String8& destAddr)
{
    destroy();
    mSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mSockFd < 0) {
        return mSockFd;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(destPort);
    addr.sin_addr.s_addr = inet_addr(destAddr.c_str());

    return ::connect(mSockFd, (struct sockaddr *)&addr, sizeof(addr));
}

ssize_t TcpClient::recv(uint8_t *buf, uint32_t bufSize)
{

}

ssize_t TcpClient::send(const uint8_t *buf, uint32_t bufSize)
{

}

void TcpClient::destroy()
{
    if (mSockFd > 0) {
        close(mSockFd);
        mSockFd = -1;
    }
}

} // namespace eular
