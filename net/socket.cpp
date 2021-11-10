/*************************************************************************
    > File Name: socket.cpp
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:45:59 PM CST
 ************************************************************************/

#include "socket.h"
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/exception.h>
#include <fcntl.h>
#include <atomic>

#define LOG_TAG "socket"
#define LISTEN_SOCKET_NUM 512

namespace Jarvis {

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
        if (clientFd > 0) {
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

} // namespace Jarvis

