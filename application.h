/*************************************************************************
    > File Name: application.h
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 05:28:12 PM CST
 ************************************************************************/

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <utils/string8.h>
#include <map>

namespace eular {
class Env {
public:
    Env();
    ~Env();
    bool init(int argc, char **argv);

    void add(const String8& key, const String8& val);
    bool has(const String8& key);
    void del(const String8& key);
    String8 getVal(const String8& key);

    void addHelp(const String8& key, const String8& desc);
    void delHelp(const String8& key);
    void printHelp() const;

    const String8& getHomeDir() const;      // 返回当前工作目录的根目录

private:
    std::map<String8, String8>  mHelpMap;   // key-help info
    std::map<String8, String8>  mArgsMap;   // key-val
    String8                     mPwd;       // 当前工作环境的根目录
    String8                     mProgName;  // 程序名字
};

class Application
{
public:
    Application();
    ~Application();

    int init(int argc, char **argv);
    int run();
private:
    int main();
    void start_with_daemon();
    void start_with_terminal();

private:
    int     argc;
    char**  argv;
    Env     env;
    String8 mConfigPath;
    bool    mRunAsDaemons;
};


} // namespace eular

#endif // __APPLICATION_H__
