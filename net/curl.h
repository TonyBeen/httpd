/*************************************************************************
    > File Name: curl.h
    > Author: hsz
    > Brief:
    > Created Time: Sat 05 Feb 2022 05:30:49 PM CST
 ************************************************************************/

#ifndef __HTTPD_NET_CURL_H__
#define __HTTPD_NET_CURL_H__

#include <utils/string8.h>
#include <curl/curl.h>
#include <unordered_map>

__attribute__((constructor))
void CurlGlobalInit();
__attribute__((destructor))
void CurlGlobalClean();

namespace eular {
class Curl
{
public:
    Curl();
    Curl(const String8 &url);
    ~Curl();

    bool setUrl(const String8 &url);

    // 追加http头部
    bool storeHeader(const String8 &key, const String8 &value);
    void clearHeader();
    bool setoptVerbose(uint8_t on);
    bool setFileds(const String8 &request);
    bool perform();

    bool isValid() const { return mCurl != nullptr; }

    String8 getResponse() const { return mResponseContent; }

protected:
    static size_t CURL_Callback(void *, size_t, size_t, void*);

protected:
    CURL *      mCurl;
    String8     mResponseContent;
    String8     mUrl;

    curl_slist *mHttpHeader;
};


} // namespace eular

#endif