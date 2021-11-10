#include "net/socket.h"
#include <log/log.h>
#include <unistd.h>

#define LOG_TAG "test-socket"

using namespace Jarvis;

void test_tcp()
{
    Socket sock(Socket::TCP, Socket::IPv4, 9000);
    if (!sock.isVaild()) {
        LOGE("socket create failed.");
        return;
    }
    sock.setnonblock();
    int clientSock = 0;
    do {
        sockaddr_in clientAddr;
        clientSock = sock.accept(&clientAddr);
    } while (clientSock < 0);
    LOGE("accept sock %d", clientSock);
    char buf[128] = {0};
    int ret = 0;
    int n = 0;
    while (n++ < 10) {
        if (ret = sock.recv(clientSock, buf, 128) > 0) {
            LOGI("recv: %s", buf);
        }
        usleep(50000);
    }
}

void test_udp()
{
    Socket sock(Socket::UDP, Socket::IPv4, 9000);
    if (!sock.isVaild()) {
        LOGE("socket create failed.");
        return;
    }
    sock.setnonblock();

    int clientSock = 0;
    do {
        sockaddr_in clientAddr;
        clientSock = sock.accept(&clientAddr);
    } while (clientSock < 0);
    LOGE("accept sock %d", clientSock);

}

int main()
{
    test_tcp();
    return 0;
}