/*************************************************************************
    > File Name: json.cpp
    > Author: hsz
    > Brief:
    > Created Time: Thu 13 Jan 2022 01:58:24 PM CST
 ************************************************************************/

#include "json.h"
#include <log/log.h>
#include <utils/exception.h>

#define LOG_TAG "json"

namespace eular {
JsonGenerator::JsonGenerator() :
    mJsonRoot(nullptr)
{
    mJsonRoot = cJSON_CreateObject();
    if (mJsonRoot == nullptr) {
        throw Exception("cJSON_CreateObject filed");
    }
}

JsonGenerator::JsonGenerator(const JsonGenerator &json)
{
    *this = json;
}

JsonGenerator::~JsonGenerator()
{
    if (mJsonRoot) {
        cJSON_Delete(mJsonRoot);
    }
}

JsonGenerator &JsonGenerator::operator=(const JsonGenerator &json)
{

}

bool JsonGenerator::AddNode(const String8 &key, const String8 &val)
{
    auto ret = mStringMap.insert(std::make_pair(key, val));
    if (ret.second) {
        cJSON *tmp = cJSON_AddStringToObject(mJsonRoot, key.c_str(), val.c_str());
        if (tmp != nullptr) {
            return true;
        }
    }

    return false;
}

bool JsonGenerator::AddNode(const String8 &key, const int &val)
{
    auto ret = mIntegerMap.insert(std::make_pair(key, val));
    if (ret.second) {
        cJSON *tmp = cJSON_AddNumberToObject(mJsonRoot, key.c_str(), val);
        if (tmp != nullptr) {
            return true;
        }
    }

    return false;
}

bool JsonGenerator::AddNode(const String8 &key, const double &val)
{
    auto ret = mDoubleMap.insert(std::make_pair(key, val));
    if (ret.second) {
        cJSON *tmp = cJSON_AddNumberToObject(mJsonRoot, key.c_str(), val);
        if (tmp != nullptr) {
            return true;
        }
    }

    return false;
}

bool JsonGenerator::AddArrayNode(const String8 &key, const std::vector<std::pair<String8, String8>> &val)
{
    cJSON *array = cJSON_CreateArray();
    cJSON *temp = cJSON_CreateObject();
    if (array == nullptr || temp == nullptr) {
        return false;
    }

    cJSON_AddItemToObject(mJsonRoot, key.c_str(), array);
    cJSON_AddItemToArray(array, temp);
    for (auto it : val) {
        cJSON_AddStringToObject(temp, it.first.c_str(), it.second.c_str());
    }

    return true;
}

bool JsonGenerator::UpdateNode(const String8 &key, const String8 &val)
{
    mStringMap[key] = val;
    return cJSON_ReplaceItemInObject(mJsonRoot, key.c_str(), cJSON_CreateString(val.c_str()));
}

bool JsonGenerator::UpdateNode(const String8 &key, const int &val)
{
    mIntegerMap[key] = val;
    return cJSON_ReplaceItemInObject(mJsonRoot, key.c_str(), cJSON_CreateNumber(val));
}

bool JsonGenerator::UpdateNode(const String8 &key, const double &val)
{
    mDoubleMap[key] = val;
    return cJSON_ReplaceItemInObject(mJsonRoot, key.c_str(), cJSON_CreateNumber(val));
}

bool JsonGenerator::DelNode(const String8 &key, JSONTYPE type)
{
    switch (type) {
    case JSONTYPE::INTEGER:
        {
            auto it = mIntegerMap.find(key);
            if (it != mIntegerMap.end()) {
                mIntegerMap.erase(it);
            }
        }
        break;
    case JSONTYPE::STRING:
        {
            auto it = mStringMap.find(key);
            if (it != mStringMap.end()) {
                mStringMap.erase(key);
            }
        }
        break;
    case JSONTYPE::DOUBLE:
        {
            auto it = mDoubleMap.find(key);
            if (it != mDoubleMap.end()) {
                mDoubleMap.erase(key);
            }
        }
        break;
    default:
        LOGW("%s() unknow type %d", __func__, (int)type);
        return false;
    }

    cJSON_DeleteItemFromObject(mJsonRoot, key.c_str());
    return true;
}

std::shared_ptr<char> JsonGenerator::dump() const
{
    std::shared_ptr<char> ptr(cJSON_Print(mJsonRoot), [](char *addr) {
        if (addr) {
            cJSON_free(addr);
        }
    });
    return ptr;
}

bool JsonGenerator::KeepFile(const String8 &path) const
{
    char *json = nullptr;
    int writeSize = 0;
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_FSYNC);
    if (fd < 0) {
        LOGE("can not save to file [%s]. [%d,%s]", path.c_str(), errno, strerror(errno));
        goto error;
    }

    json = cJSON_Print(mJsonRoot);
    if (json == nullptr) {
        LOGE("cJSON_Print error. maybe no memeory");
        goto error;
    }
    writeSize = write(fd, json, strlen(json));
    cJSON_free(json);
    return (writeSize > 0);

error:
    if (fd > 0) {
        close(fd);
    }
    return false;
}

JsonParser::JsonParser()
{
    mJsonRoot = nullptr;
}

JsonParser::JsonParser(const String8 &json) :
    mJsonRoot(nullptr)
{
    mJsonRoot = cJSON_Parse(json.c_str());
    if (mJsonRoot == nullptr) {
        throw Exception(String8::format("cJSON_Parse error."));
    }

    Parse("", mJsonRoot);
}

JsonParser::~JsonParser()
{
    if (mJsonRoot) {
        cJSON_Delete(mJsonRoot);
    }
}

bool JsonParser::Parse(const String8 &json, bool hasHttpResponse)
{
    String8 willParser = json;
    if (hasHttpResponse) {
        int index = willParser.find("\r\n\r\n");
        if (index > 0) {
            willParser = String8(json.c_str() + index + 4);
            LOGD("%s()\n%s", __func__, willParser.c_str());
        }
    }

    mJsonRoot = cJSON_Parse(willParser.c_str());
    if (mJsonRoot == nullptr) {
        return false;
    }

    mJsonMap.clear();

    Parse("", mJsonRoot);

    return true;
}

std::vector<JsonMeta> JsonParser::GetValVecByKey(const String8 &key)
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

String8 JsonParser::GetStringValByKey(const String8 &key)
{
    auto it = mJsonMap.find(key);
    if (it != mJsonMap.end() && cJSON_IsString(it->second)) {
        return it->second->valuestring;
    }

    return "";
}

int JsonParser::GetIntValByKey(const String8 &key)
{
    auto it = mJsonMap.find(key);
    if (it != mJsonMap.end() && cJSON_IsNumber(it->second)) {
        return it->second->valueint;
    }

    return INT32_MIN;
}

/**
 * @brief 解析json
 * 
 * 如下json
 * {
 *   "test": [
 *       { "info": [{"name": "eular", "age": 23, "year": 1990}] },
 *       { "info": [{"name": "alias", "age": 23, "year": 1990}] }
 *   ]
 * }
 * 
 * 解析完后为
 * test             -- cJSON *
 * test.info        -- cJSON *
 * test.info        -- cJSON *
 * test.info.name   -- cJSON *
 * test.info.name   -- cJSON *
 * test.info.age    -- cJSON *
 * test.info.age    -- cJSON *
 * test.info.year   -- cJSON *
 * test.info.year   -- cJSON *
 * 
 * 根据字符串找值
 * 
 * @param perfix 键值前导符
 * @param node   根节点的子节点，cjson的根节点无数据
 */
void JsonParser::Parse(String8 perfix, cJSON *node)
{
    String8 key = node->string ? node->string : "";
    if (cJSON_IsObject(node) == false) {
        mJsonMap.insert(std::make_pair(perfix + key, node));
    }
    if (node->next) {
        Parse(perfix, node->next);
    }

    if (node->child) {
        String8 temp = node->string ? node->string : "";
        Parse(perfix.isEmpty() ? temp :
            perfix + "." + temp, node->child);
    }
}

} // namespace eular
