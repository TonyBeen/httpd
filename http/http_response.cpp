/*************************************************************************
    > File Name: http/http_response.cpp
    > Author: hsz
    > Brief:
    > Created Time: Tue 30 Nov 2021 10:38:18 AM CST
 ************************************************************************/

#include "http_response.h"
#include <log/log.h>
#include <utils/utils.h>

#define LOG_TAG "HttpResponse"

namespace eular {

static std::map<String8, String8> gDefaultReponse = {
    {"Server", "eular/httpd v1.0"}
};


void HttpResponse::addContent(const String8 &key, const String8 &val, bool forceCover)
{
    if (isLocked) {
        return;
    }
    if (!forceCover && mHttpResBody.find(key) != mHttpResBody.end()) {
        return;
    }
    mHttpResBody[key] = val;
}

void HttpResponse::delContent(const String8 &key)
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

String8 HttpResponse::CreateHttpReponseHeader() const
{
    String8 ret;
    ret.appendFormat("%s %d %s\r\n", HttpVersion2String(mVer).c_str(), (int)mStatus, HttpStatus2String(mStatus).c_str());
    return ret;
}

String8 HttpResponse::CreateHttpReponseHeader(HttpVersion ver, HttpStatus status)
{
    if (isLocked == false) {
        mVer = ver;
        mStatus = status;
    }

    return CreateHttpReponseHeader();
}

String8 HttpResponse::CreateHttpReponseBody() const
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
    if (isLocked) {
        return;
    }

    mWillSendFilePath = fp;
    int dotIdx = fp.find('.');
    const String8 &fileExten = String8(fp.c_str() + dotIdx + 1);
    mHttpResBody.emplace(std::make_pair("Content-Type", String8::format("%s", GetContentTypeByFileExten(fileExten).c_str())));
    mHttpResBody.emplace(std::make_pair("Content-Length", String8::format("%d", GetFileLength(mWillSendFilePath))));
    mType = ResponseType::FILE;
}

void HttpResponse::setJson(const JsonGenerator &j)
{
    if (isLocked) {
        return;
    }

    mJson = j;
    mHttpResBody.emplace(std::make_pair("Content-Type", "application/json"));
    mHttpResBody.emplace(std::make_pair("Content-Length", String8::format("%zu", j.dump().length())));
    mType = ResponseType::JSON;
}

void HttpResponse::dump(String8 &msg)
{
    msg += CreateHttpReponseHeader();
    msg += CreateHttpReponseBody();
}

} // namespace eular
