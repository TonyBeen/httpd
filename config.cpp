/*************************************************************************
    > File Name: config.cpp
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 05:29:39 PM CST
 ************************************************************************/

#include "config.h"
#include <string>
#include <string.h>
#include <iostream>

#define LOG_TAG "config"

#define DEFAULT_CONFIG_PATH ""  // unused

namespace Jarvis {

namespace TypeCast {
// 模板全特化
template<>
int Chars2Other<int>(const char *src)
{
    return atoi(src);
}

template<>
unsigned int Chars2Other<unsigned int>(const char *src)
{
    return static_cast<unsigned int>(atoi(src));
}

template<>
float Chars2Other<float>(const char *src)
{
    return atof(src);
}

template<>
double Chars2Other<double>(const char *src)
{
    return static_cast<double>(atof(src));
}

template<>
long Chars2Other<long>(const char *src)
{
    return atol(src);
}

template<>
unsigned long Chars2Other<unsigned long>(const char *src)
{
    return static_cast<unsigned long>(atol(src));
}

template<>
bool Chars2Other<bool>(const char *src)
{
    bool flag;
    flag = strcasecmp(src, "true") == 0;
    return flag;
}

template<>
const char *Chars2Other<const char *>(const char *src)
{
    return src;
}
}

std::map<String8, YAML::Node> Config::mConfigMap;
Mutex Config::mMutex;

Config::Config(const String8 &path) :
    mYamlConfigPath(path),
    mValid(false),
    mErrorMsg("OK")
{
    mConfigMap.clear();
    Init();
}

Config::~Config()
{

}

void Config::Init()
{
    try {
        mYamlRoot = YAML::LoadFile(mYamlConfigPath.c_str());
        AutoLock<Mutex> lock(mMutex);
        LoadYaml("", mYamlRoot);
        mValid = true;
    } catch (const std::exception &e){
        mErrorMsg = e.what();
    }
}

void Config::LoadYaml(const String8& prefix, const YAML::Node& node)
{
    if (prefix.toStdString().find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890")
        != std::string::npos) {
        LOGE("Config invalid prefix: %s", prefix.c_str());
        return;
    }
    mConfigMap.insert(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LoadYaml(prefix.isEmpty() ? it->first.Scalar() :
                prefix + "." + it->first.Scalar(), it->second);
        }
    }
}

void Config::foreach()
{
    for (auto it = mConfigMap.begin(); it != mConfigMap.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
}

} // namespace Jarvis
