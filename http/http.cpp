/*************************************************************************
    > File Name: http.cpp
    > Author: hsz
    > Brief:
    > Created Time: Fri 29 Oct 2021 02:48:54 PM CST
 ************************************************************************/

#include "http.h"

namespace Jarvis {

static const char *gMethodString[] = {
#define XX(num, name, string)  #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

String8 HttpMethod2String(HttpMethod hm)
{
    uint32_t index = (uint32_t)hm;
    if (index >= (sizeof(gMethodString) / sizeof(gMethodString[0]))) {
        return "";
    }
    return gMethodString[index];
}

HttpMethod String2HttpMethod(String8 str)
{
#define XX(num, name, string)           \
    if (str.StrCaseCmp(#string) == 0) { \
        return HttpMethod::name;        \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

String8 HttpStatus2String(HttpStatus hs)
{
#define XX(num, name, string)           \
    if (HttpStatus::name == hs) {       \
        return #string;                 \
    }
    HTTP_STATUS_MAP(XX);
#undef XX
    return "";
}

HttpStatus String2HttpStatus(String8 str)
{
#define XX(num, name, string)               \
    if (str.StrCaseCmp(#string) == 0) {     \
        return HttpStatus::name;            \
    }
    HTTP_STATUS_MAP(XX);
#undef XX
    return HttpStatus::INVALID_STATUS;
}

String8 HttpVersion2String(HttpVersion hv)
{
#define XX(code, name, string)          \
    if (HttpVersion::name == hv) {      \
        return #string;                 \
    }
    HTTP_VERSION_MAP(XX);
#undef XX
    return "";
}

HttpVersion String2HttpVersion(String8 str)
{
#define XX(code, name, string)              \
    if (str.StrCaseCmp(#string) == 0) {     \
        return HttpVersion::name;           \
    }
    HTTP_VERSION_MAP(XX);
#undef XX
    return HttpVersion::INVALID_VERSION;
}

String8 GetContentTypeByFileExten(const String8 &fileExt)
{
#define XX(file_extension, content_type)                \
    if (fileExt.StrCaseCmp(#file_extension) == 0) {     \
        return #content_type;                           \
    }
    FILE_TYPE_MAP(XX);
#undef XX
    return "";
}

} // namespace Jarvis
