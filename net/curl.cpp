/*************************************************************************
    > File Name: curl.cpp
    > Author: hsz
    > Brief:
    > Created Time: Sat 05 Feb 2022 05:30:57 PM CST
 ************************************************************************/

#include "curl.h"
#include <log/log.h>

#define LOG_TAG "curl"

void CurlGlobalInit()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void CurlGlobalClean()
{
    curl_global_cleanup();
}

namespace eular {
Curl::Curl()
{
    mCurl = curl_easy_init();
    mHttpHeader = nullptr;
}

Curl::Curl(const String8 &url) :
    mUrl(url),
    mHttpHeader(nullptr)
{
    mCurl = curl_easy_init();
}

Curl::~Curl()
{
    clearHeader();
    if (mCurl) {
        curl_easy_cleanup(mCurl);
        mCurl = nullptr;
    }
}

bool Curl::setUrl(const String8 &url)
{
    mUrl = url;
    if (!mCurl) {
        mCurl = curl_easy_init();
    }

    return mCurl != nullptr;
}

bool Curl::storeHeader(const String8 &key, const String8 &value)
{
    String8 keyValue = String8::format("%s: %s", key.c_str(), value.c_str());
    mHttpHeader = curl_slist_append(mHttpHeader, keyValue.c_str());

    return mHttpHeader != nullptr;
}

void Curl::clearHeader()
{
    if (mHttpHeader) {
        curl_slist_free_all(mHttpHeader);
        mHttpHeader = nullptr;
    }
}

bool Curl::setoptVerbose(uint8_t on)
{
    if (!mCurl) {
        return false;
    }
    return curl_easy_setopt(mCurl, CURLOPT_VERBOSE, on) == CURLE_OK;
}

bool Curl::setFileds(const String8 &request)
{
    if (!mCurl) {
        return false;
    }

    CURLcode code = curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, request.c_str());
    if (code != CURLE_OK) {
        LOGE("curl_easy_setopt error. [%d,%s]\n", code, curl_easy_strerror(code));
        return false;
    }
    code = curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, request.length());
    if (code != CURLE_OK) {
        LOGE("curl_easy_setopt error. [%d,%s]\n", code, curl_easy_strerror(code));
        return false;
    }

    return true;
}

bool Curl::perform()
{
    if (!mCurl) {
        return false;
    }

    CURLcode code = curl_easy_setopt(mCurl, CURLOPT_URL, mUrl.c_str());
    code = curl_easy_setopt(mCurl, CURLOPT_HTTPHEADER, mHttpHeader);
    code = curl_easy_setopt(mCurl, CURLOPT_HEADER, 1);
    code = curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, this);
    code = curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, CURL_Callback);

    code = curl_easy_perform(mCurl);
    if (code != CURLE_OK) {
        LOGE("%s() curl_easy_perform error. [%d,%s]\n", __func__, code, curl_easy_strerror(code));
    }
    return code == CURLE_OK;
}

size_t Curl::CURL_Callback(void *ptr, size_t size, size_t nmem, void *userContent)
{
    if (!ptr) {
        return 0;
    }

    Curl *curl = static_cast<Curl *>(userContent);
    long length = size * nmem;
    LOG_ASSERT(curl, "never be null");

    char *recv = static_cast<char *>(ptr);
    return curl->mResponseContent.append(recv, length);
}

} // namespace eular
