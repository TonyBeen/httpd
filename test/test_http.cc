/*************************************************************************
    > File Name: ../test/test_http.cc
    > Author: hsz
    > Brief:
    > Created Time: Fri 29 Oct 2021 04:30:41 PM CST
 ************************************************************************/

#include "http/http.h"
#include <assert.h>
#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
    Jarvis::String8 strMethod = Jarvis::HttpMethod2String(Jarvis::HttpMethod::GET);
    cout << "method: " << strMethod << endl;
    Jarvis::HttpMethod intMethod = Jarvis::String2HttpMethod(strMethod);
    cout << "method GET = " << intMethod << endl;
    assert(intMethod == Jarvis::HttpMethod::GET);

    Jarvis::String8 strStatus = Jarvis::HttpStatus2String(Jarvis::HttpStatus::OK);
    cout << "status: " << strStatus << endl;
    Jarvis::HttpStatus intStatus = Jarvis::String2HttpStatus(strStatus);
    cout << "status OK = " << intStatus << endl;
    assert(intStatus == Jarvis::HttpStatus::OK);
    return 0;
}
