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
    eular::String8 strMethod = eular::HttpMethod2String(eular::HttpMethod::GET);
    cout << "method: " << strMethod << endl;
    eular::HttpMethod intMethod = eular::String2HttpMethod(strMethod);
    cout << "method GET = " << (int)intMethod << endl;
    assert(intMethod == eular::HttpMethod::GET);

    eular::String8 strStatus = eular::HttpStatus2String(eular::HttpStatus::OK);
    cout << "status: " << strStatus << endl;
    eular::HttpStatus intStatus = eular::String2HttpStatus(strStatus);
    cout << "status OK = " << (int)intStatus << endl;
    assert(intStatus == eular::HttpStatus::OK);
    return 0;
}
