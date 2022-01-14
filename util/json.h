/*************************************************************************
    > File Name: json.h
    > Author: hsz
    > Brief:
    > Created Time: Thu 13 Jan 2022 01:58:18 PM CST
 ************************************************************************/

#ifndef __EULAR_JSON_H__
#define __EULAR_JSON_H__

#include "cjson.h"
#include <utils/string8.h>
#include <map>
#include <vector>
#include <memory>
#include <typeinfo>

namespace eular {
class JsonGenerator {
public:
    JsonGenerator();
    ~JsonGenerator();

    enum class JSONTYPE {
        INTEGER = 0,
        STRING = 1,
        DOUBLE = 2
    };

    bool AddNode(const String8 &key, const String8 &val);
    bool AddNode(const String8 &key, const int &val);
    bool AddNode(const String8 &key, const double &val);
    bool AddArrayNode(const String8 &key, const std::vector<std::pair<String8, String8>> &val);

    bool UpdateNode(const String8 &key, const String8 &val);
    bool UpdateNode(const String8 &key, const int &val);
    bool UpdateNode(const String8 &key, const double &val);

    bool DelNode(const String8 &key, JSONTYPE type);

    std::shared_ptr<char> dump() const;
    bool KeepFile(const String8 &path) const;

private:
    std::map<String8, int>      mIntegerMap;    // 整型
    std::map<String8, String8>  mStringMap;     // 字符串
    std::map<String8, double>   mDoubleMap;     // 浮点

    cJSON *mJsonRoot;
};

struct JsonMeta {
    JsonMeta(cJSON *json) :
        jsonObj(json) {}

    JsonMeta &operator=(const JsonMeta &jm)
    {
        this->jsonObj = jm.jsonObj;
    }

    bool isString() { cJSON_IsString(jsonObj); }
    bool isNumber() { cJSON_IsNumber(jsonObj); }
    bool isArray() { cJSON_IsArray(jsonObj); }

    String8 getString() { cJSON_GetStringValue(jsonObj); }
    int getInt()        { (int)cJSON_GetNumberValue(jsonObj); }
    double getFloat()   { cJSON_GetNumberValue(jsonObj); }

    cJSON *jsonObj;
};

class JsonParser {
public:
    JsonParser(const String8 &json);
    ~JsonParser();

    std::vector<JsonMeta> GetValByKey(const String8 &key)
    {
        std::vector<JsonMeta> ret;
        std::multimap<String8, cJSON *>::iterator begin, end;

        begin = mJsonMap.lower_bound(key);
        end = mJsonMap.upper_bound(key);

        for (auto it = begin; it != end; ++it) {
            ret.push_back(JsonMeta(it->second));
        }

        return ret;
    }

private:
    void Parse(String8 perfix, cJSON *node);

private:
    std::multimap<String8, cJSON *>      mJsonMap;

    cJSON *mJsonRoot;
    friend class JsonGenerator;
};

} // namespace eular

#endif // __EULAR_JSON_H__
