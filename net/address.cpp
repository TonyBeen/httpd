/*************************************************************************
    > File Name: address.cpp
    > Author: hsz
    > Mail:
    > Created Time: Mon 11 Oct 2021 06:08:06 PM CST
 ************************************************************************/

#include "address.h"
#include "endian.hpp"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace eular {
Address::Address()
{

}

Address::Address(String8 ip, uint16_t port, uint32_t mask) :
    mIp(ip),
    mPort(port),
    mMask(mask)
{

}

Address::Address(const sockaddr_in &addr)
{
    *this = addr;
}

Address::Address(const Address& addr)
{
    mIp = addr.mIp;
    mPort = addr.mPort;
    mMask = addr.mMask;
}

Address &Address::operator=(const Address& addr)
{
    mIp = addr.mIp;
    mPort = addr.mPort;
    mMask = addr.mMask;

    return *this;
}

Address &Address::operator=(const sockaddr_in &addr)
{
    mIp = inet_ntoa(addr.sin_addr);
    mPort = ntohs(addr.sin_port);

    return *this;
}

Address::sp Address::CreateAddres(String8 ip, uint16_t port)
{
    Address::sp ptr(new Address(ip, port));
    return ptr;
}

// 广播地址 = 掩码取反 | 网络地址 (网络地址是大端，所以都是大端字节序)
String8 Address::GetBroadcastAddr(const String8 &addr, uint32_t mask)
{
    if (mask > 30) {
        return String8("");
    }
    uint32_t netAddr = inet_addr(addr.c_str());
    if (netAddr > 0) {
        uint32_t bitsOfRemainder = 32 - mask;
        uint32_t bitsOfMask = (1 << bitsOfRemainder) - 1;   // 掩码取反后的值
        uint32_t broadcastAddr = LittleEndian2BigEndian(bitsOfMask) | netAddr;
        in_addr inaddr;
        inaddr.s_addr = broadcastAddr;
        return inet_ntoa(inaddr);
    }

    return String8("");
}

std::ostream &operator <<(std::ostream &out, Address addr)
{
    char buf[128] = {0};
    snprintf(buf, 128, "IP: %s, Port: %u, Mask: %x\n", addr.mIp.c_str(), addr.mPort, addr.mMask);
    out << buf;
    return out;
}
} // namespace eular
