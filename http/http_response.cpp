/*************************************************************************
    > File Name: http/http_response.cpp
    > Author: hsz
    > Brief:
    > Created Time: Tue 30 Nov 2021 10:38:18 AM CST
 ************************************************************************/

#include "http_response.h"
#include <utils/utils.h>

namespace Jarvis {

static std::map<String8, String8> gDefaultReponse = {
    {"Server", "Jarvis/Httpd v1.0"},
    {"Connection", "keep-alive"}
};


void HttpResponse::addToResBody(const String8 &key, const String8 &val, bool forceCover)
{
    if (isLocked) {
        return;
    }
    if (!forceCover && mHttpResBody.find(key) != mHttpResBody.end()) {
        return;
    }
    mHttpResBody[key] = val;
}

void HttpResponse::delFromBody(const String8 &key)
{
    if (isLocked) {
        return;
    }
    auto it = mHttpResBody.find(key);
    if (it == mHttpResBody.end()) {
        return;
    }
    mHttpResBody.erase(it);
}

String8 HttpResponse::CreateHttpReponseHeader(HttpVersion ver, HttpStatus status)
{
    if (isLocked == false) {
        this->mVer = ver;
        this->mStatus = status;
    }

    String8 ret;
    ret.appendFormat("%s %d %s\r\n",
        HttpVersion2String(ver).c_str(), (int)status, HttpStatus2String(status).c_str());
    return ret;
}

String8 HttpResponse::CreateHttpReponseBody()
{
    String8 ret;
    for (const auto &it : mHttpResBody) {
        ret.appendFormat("%s: %s\r\n", it.first.c_str(), it.second.c_str());
    }
    ret.append("\r\n");
    return ret;
}

String8 HttpResponse::GetDefaultResponseByKey(const String8 &key)
{
    auto it = gDefaultReponse.find(key);
    if (it != gDefaultReponse.end()) {
        return it->second;
    }
    return key;
}

void HttpResponse::setFilePath(const String8 &fp)
{
    mWillSendFilePath = fp;
    mHttpResBody.emplace(std::make_pair("Content-Length", String8::format("%d", GetFileLength(mWillSendFilePath))));
}

} // namespace Jarvis
