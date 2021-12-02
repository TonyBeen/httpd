/*************************************************************************
    > File Name: http/http_response.h
    > Author: hsz
    > Brief:
    > Created Time: Tue 30 Nov 2021 10:38:14 AM CST
 ************************************************************************/

#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include "http.h"
#include <utils/string8.h>
#include <map>

namespace Jarvis {

class HttpResponse
{
public:
    HttpResponse(int fd) : mClientFd(fd) {}
    ~HttpResponse() {}

    bool locked() const { return isLocked; }
    bool lock() { isLocked = true; return isLocked; }
    bool unlock() { isLocked = false; return  isLocked; }

    void setHttpVersion(const HttpVersion &ver) { if(!isLocked) mVer = ver; }
    void setHttpStatus(const HttpStatus &stu) { if(!isLocked) mStatus = stu; }
    void addToResBody(const String8 &key, const String8 &val, bool forceCover = false);
    void delFromBody(const String8 &key);
    void setFilePath(const String8 &fp);

    const HttpVersion &getHttpVersion() const { return mVer; }
    const HttpStatus  &getHttpStatus()  const { return mStatus; }

    String8 CreateHttpReponseHeader(HttpVersion ver, HttpStatus status);
    String8 CreateHttpReponseBody();
    static String8 GetDefaultResponseByKey(const String8 &key);

private:
    HttpVersion     mVer;
    HttpStatus      mStatus;
    std::map<String8, String8> mHttpResBody;

    bool            isLocked;       // 上锁后无法修改成员变量
    int             mClientFd;      // 需要发送的客户端套接字
    String8         mWillSendFilePath;
};

} // namespace Jarvis

#endif // __HTTP_RESPONSE_H__
