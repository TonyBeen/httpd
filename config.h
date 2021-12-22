/*************************************************************************
    > File Name: config.h
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 05:29:33 PM CST
 ************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <utils/exception.h>
#include <log/log.h>
#include <utils/mutex.h>
#include <utils/string8.h>
#include <yaml-cpp/yaml.h>
#include <set>
#include <vector>
#include <map>
#include <list>

namespace eular {
namespace TypeCast {
template<typename T>
T Chars2Other(const char *src)
{
    T t = T();
    eular::String8 str;
    str.appendFormat("invalid type. %s", typeid(T).name());
    throw eular::bad_type_cast_exception(str);
    return t;
}
}

class Config
{
public:
    Config(const String8 &path);
    ~Config();

    bool isVaild() const { return mValid; }
    const String8& getErrorMsg() const { return mErrorMsg; }
    void foreach();

    template<typename T>
    static T Lookup(const String8& key, const T& default_val)
    {
        AutoLock<Mutex> lock(mMutex);
        auto it = mConfigMap.find(key);
        if (it != mConfigMap.end()) {
            return TypeCast::Chars2Other<T>(it->second.Scalar().c_str());
        }

        return default_val;
    }

private:
    void Init();
    void LoadYaml(const String8& prefix, const YAML::Node& node);

private:
    String8                                 mYamlConfigPath;       // yaml配置文件绝对路径
    YAML::Node                              mYamlRoot;
    static std::map<String8, YAML::Node>    mConfigMap;

    bool    mValid;
    String8 mErrorMsg;
    static Mutex   mMutex;
};

} // namespace eular

#endif // __CONFIG_H__