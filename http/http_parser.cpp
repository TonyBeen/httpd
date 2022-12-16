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

void HttpRequestParser::onRequestMethod(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mMethod = String2HttpMethod(String8(at, len));
    LOGD("%s() %s", __func__, String8(at, len).c_str());
}

void HttpRequestParser::onRequestVersion(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mVersion = String2HttpVersion(String8(at, len));
    LOGD("%s() %s", __func__, String8(at, len).c_str());
}

void HttpRequestParser::onRequestUri(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mUri = String8(at, len);
    LOGD("%s() %s", __func__, requestParser->mUri.c_str());
}

void HttpRequestParser::onRequestPath(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mPath = String8(at, len);
    LOGD("%s() %s", __func__, requestParser->mPath.c_str());
}

void HttpRequestParser::onRequestFragment(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mFragment = String8(at, len);
    LOGD("%s() %s", __func__, requestParser->mFragment.c_str());
}

// 数据在请求行会调到
void HttpRequestParser::onRequestQuery(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mRequestData = ByteBuffer(at, len);
    LOGD("%s() %s", __func__, requestParser->mRequestData.const_data());
}

// 数据在结尾会调到
void HttpRequestParser::onRequestHeaderDone(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    requestParser->mRequestData = ByteBuffer(at, len);
    LOGD("%s() %s", __func__, requestParser->mRequestData.const_data());
}

void HttpRequestParser::onRequestField(void *userData, const char *field, size_t flen, const char *value, size_t vlen)
{
    LOG_ASSERT(userData, "never be null");
    HttpRequestParser *requestParser = static_cast<HttpRequestParser *>(userData);
    String8 _field = String8(field, flen);
    String8 _value = String8(value, vlen);
    requestParser->mHttpRequestMap.insert(std::make_pair(_field, _value));
    // requestParser->mHttpRequestMap[_field] = _value;
    LOGD("%s() [%s][%s]", __func__, _field.c_str(), _value.c_str());
}

HttpRequestParser::HttpRequestParser()
{

}

HttpRequestParser::HttpRequestParser(const ByteBuffer& httpRequest)
{
    parse((const char *)httpRequest.const_data());
}

HttpRequestParser::HttpRequestParser(const String8& httpRequest)
{
    parse(httpRequest);
}

int HttpRequestParser::parse(const String8& httpRequest)
{
    // int32_t index = httpRequest.find("\r\n");
    // if (index < 0) {
    //     return 0;
    // }
    // 请求行
    // String8 requestLine(httpRequest.c_str(), index);    // 去掉/r/n的请求行
    // char method[16] = {0};
    // char url[128] = {0};
    // char version[32] = {0};
    // sscanf(requestLine.c_str(), "%s %s %s", method, url, version);
    // LOGD("method = %s, url = %s, version = %s", method, url, version);
    // mMethod = String2HttpMethod(method);
    // mVersion = String2HttpVersion(version);
    // mUri = url;
    // int32_t prevIndex = index + 2;
    // while ((index = httpRequest.find("\r\n", prevIndex)) > 0) {
    //     String8 line = String8(httpRequest.c_str() + prevIndex, index - prevIndex);
    //     // LOGD("one line of http request: %s", line.c_str());
    //     int32_t colonPos = line.find(':');
    //     if (colonPos > 0) {
    //         String8 key(line.c_str(), colonPos);
    //         String8 val(line.c_str() + colonPos + 2);
    //         LOGD("key[%s], val[%s]", key.c_str(), val.c_str());
    //         mHttpRequestMap.emplace(std::make_pair(key, val));
    //     } else { // 到达末尾\r\n
    //         index += 2;
    //         break;
    //     }
    //     prevIndex = index + 2;  // 下一行起点
    // }

    // int32_t queMarkPos;
    // switch (mMethod)
    // {
    // case HttpMethod::GET:
    //     queMarkPos =  String8::kmp_strstr(url, "?"); // ?的位置
    //     if (queMarkPos > 0) {
    //         mRequestData.set((const uint8_t *)url + queMarkPos + 1, strlen(url + queMarkPos + 1));
    //         mUri = String8(url, queMarkPos);
    //         LOGD("url reset to %s", mUri.c_str());
    //     }
    //     break;
    // case HttpMethod::POST:
    //     mRequestData.set((const uint8_t *)httpRequest.c_str() + index, httpRequest.length() - index);
    //     LOGD("POST data: [\"%s\"]", mRequestData.data());
    //     break;
    // case HttpMethod::PUT:
    //     break;
    // default:
    //     LOGW("unknow method: %s", method);
    //     break;
    // }

    // 新的解析方式
    http_request_parser_init(&mParser);
    mParser.data = this;
    mParser.request_method = onRequestMethod;
    mParser.request_uri = onRequestUri;
    mParser.fragment = onRequestFragment;
    mParser.request_path = onRequestPath;
    mParser.query_string = onRequestQuery;
    mParser.http_version = onRequestVersion;
    mParser.header_done = onRequestHeaderDone;
    mParser.http_field = onRequestField;

    size_t ret = http_request_parser_execute(&mParser, httpRequest.c_str(), httpRequest.length(), 0);
    LOGD("%s() http_request_parser_execute: %zu\n", __func__, ret);

    if (http_request_parser_has_error(&mParser)) {
        LOGD("%s() request parser has error", __func__);
        return UNKNOWN_ERROR;
    }

    ParserRequestData();
    return OK;
}

bool HttpRequestParser::ParserRequestData()
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

const String8 &HttpRequestParser::getValueByKey(const String8 &key) const
{
    static String8 ret = "";
    auto it = mRequestDataMap.find(key);
    if (it == mRequestDataMap.end()) {
        return ret;
    }

    return it->second;
}

bool HttpRequestParser::KeepAlive() const
{
    auto it = mHttpRequestMap.find("Connection");
    if (it == mHttpRequestMap.end()) {
        if (mVersion >= HttpVersion::HTTPV11) {
            return true;
        }
        return false;
    }

    if (it->second.StrCaseCmp("close") == 0) {
        return false;
    }
    return true;
}

HttpResponseParser::HttpResponseParser()
{

}

HttpResponseParser::HttpResponseParser(const ByteBuffer &buffer)
{
    parse(buffer);
}

HttpResponseParser::HttpResponseParser(const String8 &buffer)
{
    parse(buffer);
}

HttpResponseParser::~HttpResponseParser()
{

}


int HttpResponseParser::parse(const String8& httpResponse)
{
    http_response_parser_init(&mParser, this);
    mParser.http_field = onResponseField;
    mParser.http_status = onResponseStatus;
    mParser.http_reason = onResponseReason;
    mParser.http_version = onResponseVersion;
    mParser.http_data = onResponseData;

    http_response_parser_execute(&mParser, httpResponse.c_str(), httpResponse.length(), 0);
    if (http_response_parser_has_error(&mParser)) {
        LOGE("HttpResponseParser::parse() error.");
        return UNKNOWN_ERROR;
    }

    return OK;
}

int HttpResponseParser::parse(const ByteBuffer& httpResponse)
{
    parse((const char *)httpResponse.const_data());
}

String8 HttpResponseParser::getValue(const String8 &field) const
{
    auto it = mResponseFieldMap.find(field);
    if (it == mResponseFieldMap.end()) {
        return mNULL;
    }

    return it->second;
}

void HttpResponseParser::onResponseVersion(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpResponseParser *parse = static_cast<HttpResponseParser *>(userData);
    parse->mVersion = String2HttpVersion(String8(at, len));
    LOGD("%s() %s", __func__, String8(at, len).c_str());
}

void HttpResponseParser::onResponseStatus(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpResponseParser *parse = static_cast<HttpResponseParser *>(userData);
    parse->mStatus = (HttpStatus)atoi(String8(at, len).c_str());
    LOGD("%s() %s", __func__, String8(at, len).c_str());
}

void HttpResponseParser::onResponseReason(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpResponseParser *parse = static_cast<HttpResponseParser *>(userData);
    parse->mReason = String8(at, len);
    LOGD("%s() %s", __func__, String8(at, len).c_str());
}

void HttpResponseParser::onResponseData(void *userData, const char *at, size_t len)
{
    LOG_ASSERT(userData, "never be null");
    HttpResponseParser *parse = static_cast<HttpResponseParser *>(userData);
    parse->mResponseData = String8(at, len);
    LOGD("%s() %s", __func__, String8(at, len).c_str());
}

void HttpResponseParser::onResponseField(void *userData, const char *field, size_t flen, const char *value, size_t vlen)
{
    LOG_ASSERT(userData, "never be null");
    HttpResponseParser *parse = static_cast<HttpResponseParser *>(userData);
    String8 _field = String8(field, flen);
    String8 _value = String8(value, vlen);
    parse->mResponseFieldMap.insert(std::make_pair(_field, _value));
    LOGD("%s() [%s][%s]", __func__, _field.c_str(), _value.c_str());
}

} // namespace eular
