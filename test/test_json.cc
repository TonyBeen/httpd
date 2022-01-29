/*************************************************************************
    > File Name: test_json.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 29 Jan 2022 05:27:21 PM CST
 ************************************************************************/

#include "util/json.h"
#include <iostream>

using namespace std;
using namespace eular;

int main(int argc, char **argv)
{
    const char *json = "               \
    {                                  \
        \"code\": 200,                 \
        \"message\": \"success\",      \
        \"data\": {                    \
            \"country\": \"美国\",      \
            \"province\": \"内华达\",   \
            \"city\": \"拉斯维加斯\",    \
            \"service\": \"0\"         \
        }                              \
    }";
    
    // 包含数组和对象
    json = "\
    {\
        \"test\":[{\
                \"info\":[{\
                    \"name\":\"eular\",\
                    \"age\":23,\
                    \"year\":1990\
                }]\
            },\
            {\
                \"info\":[{\
                    \"name\":\"alias\",\
                    \"age\":23,\
                    \"year\":1990\
                }]\
        }]\
    }";

    // 解析出错
    // json = "{\
    //     \"tokenColors\": [  \
    //         {   \
    //             \"name\": \"Comment\",     \
    //             \"scope\": [\"comment\"],   \
    //             \"settings\": {             \
    //                 \"foreground\": \"#5C6370\", \
    //                 \"fontStyle\": \"italic\" \
    //             }       \
    //         },          \
    //         {           \
    //             \"name\": \"Comment Markup Link\",\
    //             \"scope\": [\"comment markup.link\"],\
    //             \"settings\": {\
    //                 \"foreground\": \"#5C6370\"\
    //         }\
    //     ]\
    // }";

    JsonParser jp;
    cout << jp.Parse(json) << endl;
    // cout << jp.GetIntValByKey("code") << endl;
    // cout << jp.GetStringValByKey("message") << endl;
    // cout << jp.GetStringValByKey("data.country") << endl;
    // cout << jp.GetStringValByKey("data.province") << endl;
    // cout << jp.GetStringValByKey("data.city") << endl;
    // cout << jp.GetStringValByKey("data.service") << endl;

    return 0;
}
