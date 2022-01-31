/*************************************************************************
    > File Name: address.h
    > Author: hsz
    > Mail:
    > Created Time: Mon 11 Oct 2021 06:08:01 PM CST
 ************************************************************************/

#ifndef __ADDRESS_H__
#define __ADDRESS_H__

#include <utils/string8.h>
#include <memory>

/**
 * 提供地址和端口号供socket调用
 */
namespace eular {
class Address : public std::enable_shared_from_this<Address>
{
    friend class Epoll;
    friend class Socket;
public:
    typedef std::shared_ptr<Address> sp;
    Address();
    Address(String8 ip, uint16_t port, uint32_t mask = 0);
    Address(const sockaddr_in &addr);
    Address(const Address& addr);
    Address &operator=(const Address& addr);
    Address &operator=(const sockaddr_in &addr);

    void setIP(String8 ip) { mIp = ip; }
    void setPort(uint16_t port) { mPort = port; }
    void setMask(uint32_t mask) { mMask = mask; }

    String8 getIP() const { return mIp; }
    uint16_t getPort() const { return mPort; }

    static sp CreateAddres(String8 ip, uint16_t port);

    /**
     * @param addr IP地址
     * @param mask 表示掩码位数
     */
    static String8 GetBroadcastAddr(const String8 &addr, uint32_t mask);

private:
    friend std::ostream &operator <<(std::ostream &out, Address addr);
    String8  mIp;
    uint16_t mPort;
    uint32_t mMask;
};

std::ostream &operator <<(std::ostream &out, Address addr);

} // namespace eular

#endif // __ADDRESS_H__