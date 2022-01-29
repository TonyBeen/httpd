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
    JsonGenerator(const JsonGenerator &json);
    ~JsonGenerator();

    JsonGenerator &operator=(const JsonGenerator &json);

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

    String8 dump() const;
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
    JsonParser();
    JsonParser(const String8 &json);
    ~JsonParser();

    /**
     * @brief 解析字符串json，如果字符串包含http响应，则hasHttpResponse为true
     * 
     * @param json 要解析的json字符串
     * @param hasHttpResponse 是否存在http响应或请求
     * @return true 解析成功
     * @return false 失败
     */
    bool Parse(const String8 &json, bool hasHttpResponse = false);

    std::vector<JsonMeta> GetValVecByKey(const String8 &key);
    String8 GetStringValByKey(const String8 &key);
    int GetIntValByKey(const String8 &key);

private:
    void Parse(String8 perfix, cJSON *node);

private:
    std::multimap<String8, cJSON *>      mJsonMap;
    cJSON *mJsonRoot;
};

} // namespace eular

#endif // __EULAR_JSON_H__
