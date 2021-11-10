/*************************************************************************
    > File Name: socket.h
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:45:52 PM CST
 ************************************************************************/

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <utils/string8.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>

namespace Jarvis {
class Epoll;
class Socket
{
    friend class Epoll;
public:
    enum ProtocolType {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
    };
    enum SockFamily {
        IPv4 = AF_INET,
        UNIX = AF_UNIX, // = AF_LOCAL
    };

    Socket();
    Socket(ProtocolType protocol, SockFamily family, uint16_t port, const String8 &ip = "");
    ~Socket();

    void    setProtocol(ProtocolType protocol) { mProtocolType = protocol; }
    void    setFamily(SockFamily family) { mFamily = family; }
    void    setPort(uint16_t port) { mPort = port; }
    void    setIP(const char *ip) { mIPAddr = ip; }
    int     ResetSocket();

    int     setnonblock();
    int     getsockopt(int level, int optname, void *optval, socklen_t *optlen);
    int     setsockopt(int level, int optname, const void *optval, socklen_t optlen);

    int     accept(sockaddr_in *addr);
    int     close(int fd);

    ssize_t recv(int sock, char *buf, uint32_t bufSize);
    ssize_t send(int sock, const char *buf, uint32_t size);

    sockaddr_in getClientAddr(uint32_t clientSock);
    int     getListenSocket() const { return mSockFd; }
    bool    isVaild() const { return mValid; }
private:
    Socket(const Socket&) = delete;
    void InitSocket();
    void CreateUDP();
    void CreateTCP();
    void destroy();

private:
    ProtocolType    mProtocolType;
    SockFamily      mFamily;

    int             mSockFd;
    String8         mIPAddr;
    uint16_t        mPort;

    bool            mValid;
    std::map<uint32_t, sockaddr_in>  mClientFdMap;
};

/**
 * @brief 套接字客户端类
 */
class ClientBase
{
public:
    ClientBase(uint16_t destPort, const char *destAddr);
    virtual ~ClientBase() {}

    void        setDestPort(uint16_t port) { mDestPort = port; }
    void        setDestAddr(const char *destAddr) { mDestAddr = destAddr; }
    void        setDestAddr(const String8& destAddr) { mDestAddr = destAddr; }
    uint16_t    getPort() const { return mDestPort; }
    String8     getAddr() const { return mDestAddr; }

    virtual int connect() = 0;

    virtual ssize_t recv() = 0;
    virtual ssize_t send() = 0;

protected:
    uint16_t    mDestPort;
    String8     mDestAddr;
    sockaddr_in mSockAddr;
    socklen_t   mSockLen;
};


class ClientTCP : public ClientBase
{
public:
    ClientTCP(uint16_t destPort, const char *destAddr = nullptr);
};

class ClientUDP : public ClientBase
{
public:

};

} // namespace Jarvis

#endif