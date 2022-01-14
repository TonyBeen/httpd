/*************************************************************************
    > File Name: socket.h
    > Author: hsz
    > Mail:
    > Created Time: Sun 03 Oct 2021 05:45:52 PM CST
 ************************************************************************/

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "epoll.h"
#include "thread/thread.h"
#include <utils/utils.h>
#include <utils/string8.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <memory>

namespace eular {
class Socket
{
    DISALLOW_COPY_AND_ASSIGN(Socket);
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
    Socket(ProtocolType protocol, SockFamily family, uint16_t port, const String8 &ip = "127.0.0.1");
    virtual ~Socket();

    void    setProtocol(ProtocolType protocol) { mProtocolType = protocol; }
    void    setFamily(SockFamily family) { mFamily = family; }
    void    setPort(uint16_t port) { mPort = port; }
    void    setIP(const char *ip) { mIPAddr = ip; }
    int     ResetSocket();

    int     setnonblock();
    int     getsockopt(int level, int optname, void *optval, socklen_t *optlen);
    int     setsockopt(int level, int optname, const void *optval, socklen_t optlen);

    virtual int accept(sockaddr_in *addr);
    virtual int close(int fd);

    ssize_t recv(int sock, char *buf, uint32_t bufSize);
    ssize_t send(int sock, const char *buf, uint32_t size);

    sockaddr_in getClientAddr(uint32_t clientSock);
    int     getListenSocket() const { return mSockFd; }
    bool    isVaild() const { return mValid; }

private:
    void InitSocket();
    void CreateUDP();
    void CreateTCP();
    void destroy();

protected:
    ProtocolType    mProtocolType;
    SockFamily      mFamily;

    int             mSockFd;
    String8         mIPAddr;
    uint16_t        mPort;

    bool            mValid;
    std::map<uint32_t, sockaddr_in>     mClientFdMap;
};

class TcpServer : public Socket
{
public:
    TcpServer(uint16_t, const String8 &);
    virtual ~TcpServer();

    virtual int accept(sockaddr_in *) override;
    virtual int accept_loop();

    class Compare {
    public:
        bool operator()(const std::shared_ptr<Thread> &left, const std::shared_ptr<Thread> right)
        {
            if (left.get() < right.get()) {
                return true;
            }
            return false;
        }
    };
    
private:
    std::map<std::shared_ptr<Thread>, std::shared_ptr<Epoll>, TcpServer::Compare> mEpollProcMap;
};


/**
 * @brief 套接字客户端类
 */
class ClientBase {
public:
    ClientBase() {}
    ClientBase(uint16_t destPort, const String8& destAddr) :
        mDestPort(destPort),
        mDestAddr(destAddr),
        mSockFd(-1) {}
    virtual ~ClientBase() {}

    void        setDestPort(uint16_t port) { mDestPort = port; }
    void        setDestAddr(const String8& destAddr) { mDestAddr = destAddr; }
    uint16_t    getPort() const { return mDestPort; }
    String8     getAddr() const { return mDestAddr; }

    virtual int connect() = 0;
    virtual int reset(uint16_t destPort, const String8& destAddr) = 0;

    virtual ssize_t recv(uint8_t *buf, uint32_t bufSize) = 0;
    virtual ssize_t send(const uint8_t *buf, uint32_t bufSize) = 0;

protected:
    uint16_t    mDestPort;
    String8     mDestAddr;

    int         mSockFd;
};

class TcpClient : public ClientBase {
public:
    TcpClient(uint16_t destPort, const String8& destAddr);
    virtual ~TcpClient();

    virtual int connect() override;
    virtual int reset(uint16_t destPort, const String8& destAddr) override;

    virtual ssize_t recv(uint8_t *buf, uint32_t bufSize) override;
    virtual ssize_t send(const uint8_t *buf, uint32_t bufSize) override;

private:
    void destroy();
};

} // namespace eular

#endif