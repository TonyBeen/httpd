/*************************************************************************
    > File Name: http_parser.h
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 09:20:38 AM CST
 ************************************************************************/

#ifndef __HTTP_PASER_H__
#define __HTTP_PASER_H__

#include "http.h"
#include <http_parser/http_parser.h>
#include <utils/Buffer.h>
#include <utils/string8.h>
#include <map>
#include <memory>

namespace eular {

class HttpRequestParser
{
public:
    typedef std::map<String8, String8> HttpRquestMap;
    HttpRequestParser();
    HttpRequestParser(const ByteBuffer& httpRequest);
    HttpRequestParser(const String8& httpRequest);
    ~HttpRequestParser() {}

    int   parse(const String8& httpRequest);

    const HttpMethod  &getMethod() const { return mMethod; }
    const String8     &getUrl() const { return mUri; }
    const HttpVersion &getVersion() const { return mVersion; }
    const ByteBuffer  &getRequest() const { return mRequestData; }
    const HttpRquestMap &getRequestMap() const { return mRequestDataMap; }
    const String8     &getValueByKey(const String8 &key) const;
    bool  KeepAlive() const;

protected:
    static void onRequestMethod(void *userData, const char *at, size_t len);
    static void onRequestVersion(void *userData, const char *at, size_t len);
    static void onRequestUri(void *userData, const char *at, size_t len);
    static void onRequestPath(void *userData, const char *at, size_t len);
    static void onRequestFragment(void *userData, const char *at, size_t len);
    static void onRequestQuery(void *userData, const char *at, size_t len);
    static void onRequestHeaderDone(void *userData, const char *at, size_t len);
    static void onRequestField(void *userData, const char *field, size_t flen, const char *value, size_t vlen);

private:
    bool  ParserRequestData();

    HttpMethod          mMethod;
    HttpVersion         mVersion;
    String8             mUri;               // get情况下包含请求数据，post情况下和mPath一致
    String8             mPath;              // 请求路径
    String8             mFragment;
    ByteBuffer          mRequestData;       // 保存post或get数据
    
    HttpRquestMap       mHttpRequestMap;
    HttpRquestMap       mRequestDataMap;    // 解析出的post或get键值对
    http_request_parser mParser;
};

} // namespace eular

#endif // __HTTP_PASER_H__
