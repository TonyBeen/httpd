/*************************************************************************
    > File Name: api.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 31 Jan 2022 04:16:40 PM CST
 ************************************************************************/

#include "api.h"
#include <log/log.h>
#include <utils/Errors.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define LOG_TAG "api"

namespace eular {
namespace api {

TcpClient::TcpClient() :
    mSocket(-1),
    mRemoteHost(INADDR_NONE)
{

}

TcpClient::TcpClient(const sockaddr_in *addr) :
    mSocket(-1),
    mRemotePort(ntohs(addr->sin_port)),
    mRemoteHost(addr->sin_addr.s_addr),
    mRemoteIP(inet_ntoa(addr->sin_addr))
{
    mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (mSocket < 0) {
        return;
    }

    if (::connect(mSocket, (const sockaddr *)addr, sizeof(sockaddr_in)) < 0) {
        LOGE("%s(const sockaddr_in *addr) connect error. [%d,%s]", __func__, errno, strerror(errno));
    }
}

TcpClient::TcpClient(const String8 &host, uint16_t port) :
    mSocket(-1),
    mRemotePort(port)
{
    hostent *hostInfo = gethostbyname(host.c_str());
    if (hostInfo == nullptr) {
        return;
    }

    mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (mSocket < 0) {
        return;
    }

    char *temp = hostInfo->h_addr_list[0];
    mRemoteIP = inet_ntoa(*(struct in_addr*)temp);
    mRemoteHost = ((struct in_addr*)temp)->s_addr;
    LOGD("[%s,%u]", mRemoteIP.c_str(), mRemoteHost);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mRemotePort);
    addr.sin_addr.s_addr = mRemoteHost;

    if (::connect(mSocket, (const sockaddr *)&addr, sizeof(sockaddr_in)) < 0) {
        LOGE("%s(const String8 &host, uint16_t port) connect error. [%d,%s]", __func__, errno, strerror(errno));
    } else {
        LOGD("API %s[%s] connected", host.c_str(), mRemoteIP.c_str());
    }
}

TcpClient::TcpClient(uint16_t port, const String8& ip) :
    mSocket(-1),
    mRemotePort(port),
    mRemoteHost(inet_addr(ip.c_str())),
    mRemoteIP(ip)
{
    mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (mSocket < 0) {
        return;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(mSocket, (const sockaddr *)&addr, sizeof(sockaddr_in)) < 0) {
        LOGE("%s(uint16_t port, const String8& ip) connect error. [%d,%s]", __func__, errno, strerror(errno));
    }
}

TcpClient::~TcpClient()
{
    if (mSocket > 0 ) {
        close(mSocket);
        mSocket = -1;
    }
}

void TcpClient::setPort(uint16_t port)
{
    mRemotePort = port;
}

void TcpClient::setHost(const String8 &host)
{
    struct hostent *hostInfo = gethostbyname(host.c_str());
    if (hostInfo == nullptr) {
        return;
    }

    char *temp = hostInfo->h_addr_list[0];
    mRemoteIP = inet_ntoa(*(struct in_addr*)temp);
    mRemoteHost = ((struct in_addr*)temp)->s_addr;
}

bool TcpClient::connect(const String8 &host, uint16_t port)
{
    struct hostent *hostInfo = gethostbyname(host.c_str());
    if (hostInfo == nullptr) {
        return false;
    }

    char *temp = hostInfo->h_addr_list[0];
    mRemoteIP = inet_ntoa(*(struct in_addr*)temp);
    mRemoteHost = ((struct in_addr*)temp)->s_addr;
    mRemotePort = port;

    if (mSocket > 0) {
        close(mSocket);
        mSocket = -1;
    }

    mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (mSocket < 0) {
        return false;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = mRemoteHost;

    if (::connect(mSocket, (const sockaddr *)&addr, sizeof(sockaddr_in)) < 0) {
        LOGE("%s() connect error. [%d,%s]", __func__, errno, strerror(errno));
        return false;
    }

    LOGI("connect to [%s:%u]", mRemoteIP.c_str(), mRemotePort);
    return true;
}

bool TcpClient::setnonblock()
{
    if (mSocket < 0) {
        return false;
    }

    uint32_t flags = fcntl(mSocket, F_GETFL);
    if (flags < 0) {
        LOGE("%s() fcntl error. [%d,%s]", __func__, errno, strerror(errno));
        return false;
    }
    flags |= O_NONBLOCK;
    return fcntl(mSocket, F_SETFL, flags) == 0;
}

bool TcpClient::setrecvtimeout(uint32_t ms)
{
    if (mSocket < 0) {
        return false;
    }

    struct timeval tv{ms / 1000, ms % 1000 * 1000};
    return setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, (socklen_t)sizeof(timeval)) == 0;
}

bool TcpClient::setsendtimeout(uint32_t ms)
{
    if (mSocket < 0) {
        return false;
    }
    struct timeval tv{ms / 1000, ms % 1000 * 1000};
    return setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, (socklen_t)sizeof(timeval)) == 0;
}

int TcpClient::send(const void *buffer, uint32_t len)
{
    if (mSocket < 0) {
        return eular::NO_INIT;
    }
    int ret = ::send(mSocket, buffer, len, 0);
    if (ret < 0) {
        LOGW("send error. [%d,%s]", errno, strerror(errno));
        close(mSocket);
        mSocket = -1;
    }
    return ret;
}

int TcpClient::send(const ByteBuffer &buffer)
{
    if (mSocket < 0) {
        return eular::NO_INIT;
    }

    int ret = ::send(mSocket, buffer.const_data(), buffer.size(), 0);
    if (ret < 0) {
        LOGW("send error. [%d,%s]", errno, strerror(errno));
        close(mSocket);
        mSocket = -1;
    }
    return ret;
}

int TcpClient::recv(void *buffer, uint32_t bufSize)
{
    if (mSocket < 0) {
        return eular::NO_INIT;
    }

    return ::recv(mSocket, buffer, bufSize, 0);
}

int TcpClient::recv(ByteBuffer &buffer)
{
    if (mSocket < 0) {
        return eular::NO_INIT;
    }

    ssize_t recvSize = ::recv(mSocket, buffer.data(), buffer.capacity(), 0);
    if (recvSize > 0) {
        buffer.setDataSize(recvSize);
    }

    return recvSize;
}

}   // namespace api
}   // namespace eular