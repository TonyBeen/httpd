#include "config.h"
#include <iostream>

using namespace Jarvis;
using namespace std;

void test_static_func()
{
    cout << "****log.level = " << Config::Lookup<const char *>("log.level", "debug") << endl;
}

int main()
{
    Config cfg("./test.yaml");
    if (cfg.isVaild() == false) {
        cout << cfg.getErrorMsg() << endl;
        return 0;
    }

    cfg.foreach();
    cout << endl;
    cout << "log.level = " << cfg.Lookup<const char *>("log.level", "debug") << endl;
    cout << "tcp.port = " << Config::Lookup<uint32_t>("tcp.port", 80) << endl;
    test_static_func();
    return 0;
}