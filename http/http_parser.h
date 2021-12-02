/*************************************************************************
    > File Name: http_parser.h
    > Author: hsz
    > Mail:
    > Created Time: Thu 16 Sep 2021 09:20:38 AM CST
 ************************************************************************/

#ifndef __HTTP_PASER_H__
#define __HTTP_PASER_H__

#include "http.h"
#include <utils/Buffer.h>
#include <utils/string8.h>
#include <map>
#include <memory>

namespace Jarvis {

class HttpParser
{
public:
    typedef std::map<String8, String8> HttpRquestMap;
    HttpParser(const ByteBuffer& httpRequest);
    HttpParser(const String8& httpRequest);
    ~HttpParser() {}

    const HttpMethod  &getMethod() const { return mMethod; }
    const String8     &getUrl() const { return mUrl; }
    const HttpVersion &getVersion() const { return mVersion; }
    const ByteBuffer  &getRequest() const { return mRequestData; }
    const HttpRquestMap &getRequestMap() const { return mRequestDataMap; }
    bool  KeepAlive() const;

private:
    const HttpRquestMap &ParserRequest(const String8& httpRequest);
    bool  ParserRequestData();

    HttpMethod      mMethod;
    String8         mUrl;
    HttpVersion     mVersion;
    HttpRquestMap   mHttpRequestMap;
    ByteBuffer      mRequestData;       // 保存post或get数据
    HttpRquestMap   mRequestDataMap;    // 解析出的post或get键值对
};

} // namespace Jarvis

#endif // __HTTP_PASER_H__
