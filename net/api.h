/*************************************************************************
    > File Name: api.h
    > Author: hsz
    > Brief: 提供查询接口，如获取IP所属位置
    > Created Time: Mon 31 Jan 2022 04:16:35 PM CST
 ************************************************************************/

#ifndef __NET_API_H__
#define __NET_API_H__

#include <utils/Buffer.h>
#include <utils/string8.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory>

namespace eular {
namespace api {

class ClientBase
{
public:
    ClientBase() {}
    virtual ~ClientBase() {}

    virtual bool setnonblock() = 0;
    virtual bool setrecvtimeout(uint32_t) = 0;
    virtual bool setsendtimeout(uint32_t) = 0;

    virtual int send(const void *buffer, uint32_t len) = 0;
    virtual int send(const ByteBuffer &buffer) = 0;
    virtual int recv(void *buffer, uint32_t bufSize) = 0;
    virtual int recv(ByteBuffer &buffer) = 0;
};

class TcpClient : public ClientBase
{
public:
    TcpClient();
    TcpClient(const sockaddr_in *addr);
    TcpClient(const String8 &host, uint16_t port);
    TcpClient(uint16_t port, const String8& ip);
    virtual ~TcpClient();

    static std::shared_ptr<TcpClient> Create(uint16_t port, const String8 &remoteHost);

    void setPort(uint16_t port);
    void setHost(const String8 &host);

    bool setnonblock() override;
    bool setrecvtimeout(uint32_t) override;
    bool setsendtimeout(uint32_t) override;

    int send(const void *buffer, uint32_t len) override;
    int send(const ByteBuffer &buffer) override;
    int recv(void *buffer, uint32_t bufSize) override;
    int recv(ByteBuffer &buffer) override;

protected:
    int         mSocket;

    uint16_t    mRemotePort;
    uint32_t    mRemoteHost;
    String8     mRemoteIP;
};

}
}

#endif // __NET_API_H__