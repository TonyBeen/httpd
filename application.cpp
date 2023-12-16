/*************************************************************************
    > File Name: application.cpp
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 05:28:24 PM CST
 ************************************************************************/

#include "application.h"
#include "config.h"
#include "net/socket.h"
#include <log/log.h>
#include <log/callstack.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <atomic>

#define LOG_TAG "application"

namespace eular {
static String8 root;
static uint64_t gRestartCount = 0;
static String8 gLocalAddress = getLocalAddress()[0];
static uint16_t gPort;
static uint32_t gFatherPid = 0;
static uint32_t gSonPid = 0;

Application::Application() :
    mRunAsDaemons(false)
{

}

Application::~Application()
{

}

int Application::init(int argc, char **argv)
{
    this->argc = argc;
    this->argv = argv;

    env.init(argc, argv);
    env.addHelp("-d", "run as daemon");
    env.addHelp("-c", "-c /path/to/config");
    env.addHelp("-p", "print help");

    if (env.has("p")) {
        env.printHelp();
        exit(0);
    }
    if (env.has("c")) {
        mConfigPath = env.getVal("c");
    }
    if (env.has("d")) {
        mRunAsDaemons = true;
    }

    Config cfg(mConfigPath);
    if (cfg.isVaild() == false) {
        env.printHelp();
        LOGE("%s", cfg.getErrorMsg().c_str());
        exit(0);
    }
    String8 level = cfg.Lookup<const char *>("log.level", "debug");
    LogLevel::Level lev = LogLevel::String2Level(level.c_str());
    bool sync = cfg.Lookup<bool>("log.sync", true);
    root = cfg.Lookup<const char *>("html.root", "/home/eular/vscode/www/html/");
    String8 target = cfg.Lookup<const char *>("log.target", "stdout");
    bool stdOut = target.contains("stdout");
    bool fileOut = target.contains("fileout");
    bool consoleOut = target.contains("consoleout");
    log::InitLog(lev);
    log::SetPath("/home/eular/log/");
    if (fileOut) {
        log::addOutputNode(LogWrite::FILEOUT);
    }
    if (consoleOut) {
        log::addOutputNode(LogWrite::CONSOLEOUT);
    }
    if (stdOut == false) {
        log::delOutputNode(LogWrite::STDOUT);
    }

    gPort = Config::Lookup<uint32_t>("tcp.port", 80);

    LOGI("load config file over");
    LOGI("log level = %s; log target = %s; log sync = %s",
        level.c_str(), target.c_str(), sync ? "true" : "false");
    return OK;
}

int Application::run()
{
    // TODO 判断一些html是否存在，如index.html home.html 404.html
    LOGI("start as daemon %d", mRunAsDaemons);
    if (mRunAsDaemons) {
        start_with_daemon();
    } else {
        start_with_terminal();
    }
    return 0;
}

void Signalcatch(int sig)
{
    LOGI("catch signal %d", sig);
    if (sig == SIGSEGV) {
        // 产生堆栈信息;
        CallStack stack;
        stack.update();
        stack.log("SIGSEGV", LogLevel::LEVEL_FATAL);
        exit(0);
    }

    if (sig == SIGABRT) {
        // 产生堆栈信息;
        CallStack stack;
        stack.update();
        stack.log("SIGABRT", LogLevel::LEVEL_FATAL);
        exit(0);
    }

    if (sig == SIGUSR1) {
        if (gettid() == gFatherPid) {
            kill(gSonPid, SIGKILL);
        } else if (gettid() == gSonPid) {
            exit(SIGKILL);
        }
    }
}

int Application::main()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGABRT, Signalcatch);
    signal(SIGSEGV, Signalcatch);
    signal(SIGUSR1, Signalcatch);

    TcpServer tcpServer(gPort, gLocalAddress);
    while (true) {
        int status = tcpServer.accept_loop();
        if (mRunAsDaemons) {
            break;
        }

        if (status == Thread::THREAD_WAITING) {
            msleep(1000);
        }
    }
    return 0;
}

void Application::start_with_daemon()
{
    daemon(1, 0);
    while (true) {
        pid_t pid = fork();
        if (pid == 0) {  // 子进程
            main();
        } else if (pid < 0) {
            LOGE("fork failed return %d errno %d errstr = %s", pid, errno, strerror(errno));
            return;
        } else if (pid > 0) { // 父进程
            int status = 0;
            gFatherPid = gettid();
            gSonPid = pid;
            waitpid(pid, &status, 0);
            if (status) {
                if (status == SIGKILL) {
                    LOGI("pid %d killed", pid);
                    break;
                } else {
                    LOGE("Subprocess exit with %d", status);
                }
            }
            gRestartCount++;
        }
    }
    LOGI("restart count = %llu", gRestartCount);
}

void Application::start_with_terminal()
{
    main();
}

Env::Env()
{

}

Env::~Env()
{

}

void Env::init(int argc, char **argv)
{
    char link[128] = {0};
    char path[128] = {0};
    sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, path, sizeof(path));
    std::string exe = path;
    size_t pos = exe.find_last_of('/');
    mPwd = String8(path, pos + 1);
    mProgName = argv[0];

    for (int i = 1; i < argc; ++i) {
        int pos = String8(argv[i]).find('-');
        if (pos < 0) {
            continue;
        }
        char which = *(argv[i] + pos + 1);
        switch (which) {
        case 'p':   // -p 打印帮助信息
            add("p", "");
            break;
        case 'c':   // -c /path/to/config_file 配置文件路径
            if (i + 1 < argc && String8(argv[i + 1]).find('-') < 0) {
                add("c", argv[i + 1]);
            }
            break;
        case 'd':   // -d 以守护进程启动
            add("d", "");
            break;
        default:
            break;
        }
    }
}

void Env::add(const String8& key, const String8& val)
{
    mArgsMap.insert(std::make_pair(key, val));
}

bool Env::has(const String8& key)
{
    if (mArgsMap.size() <= 0) {
        return false;
    }
    auto it = mArgsMap.find(key);
    if (it != mArgsMap.end()) {
        return true;
    }
    return false;
}

void Env::del(const String8& key)
{
    mArgsMap.erase(key);
}

String8 Env::getVal(const String8& key)
{
    auto it = mArgsMap.find(key);
    if (it != mArgsMap.end()) {
        return it->second;
    }
    return "";
}

void Env::addHelp(const String8& key, const String8& desc)
{
    mHelpMap[key] = desc;
}

void Env::delHelp(const String8& key)
{
    mHelpMap.erase(key);
}

void Env::printHelp() const
{
    LOGI("%s", mProgName.c_str());
    for (auto it = mHelpMap.begin(); it != mHelpMap.end(); ++it) {
        LOGI("%s %s\n", it->first.c_str(), it->second.c_str());
    }
}

const String8& Env::getHomeDir() const
{
    return mPwd;
}


} // namespace eular

