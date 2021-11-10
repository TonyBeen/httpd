/*************************************************************************
    > File Name: test_address.cc
    > Author: hsz
    > Mail:
    > Created Time: Mon 18 Oct 2021 03:58:17 PM CST
 ************************************************************************/

#include <iostream>
#include "net/address.h"

using namespace std;

int main(int argc, char **argv)
{
    using namespace Jarvis;
    cout << Address::GetBroadcastAddr("192.168.23.143", 24) << endl;    // 192.168.23.255
    cout << Address::GetBroadcastAddr("202.112.14.137", 27) << endl;    // 202.112.14.159

    return 0;
}
