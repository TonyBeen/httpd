/*************************************************************************
    > File Name: http/http_response.h
    > Author: hsz
    > Brief:
    > Created Time: Tue 30 Nov 2021 10:38:14 AM CST
 ************************************************************************/

#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include "http.h"
#include "util/json.h"
#include <utils/string8.h>
#include <map>

namespace eular {

class HttpResponse
{
public:
    enum ResponseType {
        FILE = 0,   // 响应格式为文件，包含html，css，js等文件
        JSON = 1,   // 响应为json数据
        XML  = 2,   // 响应为xml数据
        YAML = 3,   // 响应为yaml数据
        UNKNOW
    };
    HttpResponse(int fd) : mClientFd(fd), isLocked(false), mType(UNKNOW) {}
    ~HttpResponse() {}
    HttpResponse(const HttpResponse &v) = delete;
    HttpResponse &operator=(const HttpResponse &v) = delete;

    bool locked() const { return isLocked; }
    void lock() { isLocked = true; }
    void unlock() { isLocked = false; }

    void setResponseType(ResponseType type) { mType = type; }
    void setHttpVersion(const HttpVersion &ver) { if(!isLocked) mVer = ver; }
    void setHttpStatus(const HttpStatus &stu) { if(!isLocked) mStatus = stu; }
    void addContent(const String8 &key, const String8 &val, bool forceCover = false);
    void delContent(const String8 &key);
    void setFilePath(const String8 &fp);
    void setJson(const JsonGenerator &j);

    const int32_t       &getClientSocket() const { return mClientFd; }
    const String8       &getFilePath() const { return mWillSendFilePath; }
    const String8       &getJson() const { return mJson.dump(); }
    const HttpVersion   &getHttpVersion() const { return mVer; }
    const HttpStatus    &getHttpStatus()  const { return mStatus; }
    const ResponseType  &getResponseType() const { return mType; }

    String8 CreateHttpReponseHeader() const;
    String8 CreateHttpReponseHeader(HttpVersion ver, HttpStatus status);
    String8 CreateHttpReponseBody() const;
    static String8 GetDefaultResponseByKey(const String8 &key);

    void dump(String8 &msg);

private:
    HttpVersion     mVer;
    HttpStatus      mStatus;
    std::map<String8, String8> mHttpResBody;

    ResponseType    mType;

    JsonGenerator   mJson;
    bool            isLocked;       // 上锁后无法修改成员变量
    int             mClientFd;      // 需要发送的客户端套接字
    String8         mWillSendFilePath;
};

} // namespace eular

#endif // __HTTP_RESPONSE_H__
