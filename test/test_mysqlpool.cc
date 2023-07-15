/*************************************************************************
    > File Name: test/test_mysqlpool.cc
    > Author: hsz
    > Brief:
    > Created Time: Sun 21 Nov 2021 02:43:51 PM CST
 ************************************************************************/

#include "sql/mysqlpool.h"
#include <utils/errors.h>
#include <utils/thread.h>
#include <log/log.h>
#include <assert.h>
#include <iostream>

using namespace std;
using namespace eular;

#define LOG_TAG "mysqlpool test"

MysqlPool *gMysqlPool = new MysqlPool(2);

int thread(void *arg)
{
    while (1) {
        MySqlConn *conn = gMysqlPool->GetAConnection();
        if (conn == nullptr) {
            LOGI("in thread() conn is nullptr");
            sleep(1);
            continue;
        }
        if (conn->SelectSql("userinfo", nullptr, "user_name = \"abc\"") == OK) {
            SqlResBase::sp res = conn->getSqlRes();
            if (res->isVaild()) {
                int fields = res->getColumnCount();
                LOGI("fields = %d", fields);
                for (int i = 0; i < fields; ++i) {
                    cout << conn->getFieldByIdx(i);
                    cout << "\t";
                }
                cout << endl;
                while (res->next()) {
                    for (int i = 0; i < fields; ++i) {
                        cout << res->getString(i) << "\t";
                    }
                    cout << endl;
                }
            } else {
                LOGE("Unable to get resource");
            }
        }
        gMysqlPool->FreeAConnection(conn);
        sleep(1);
    }
}

int main(int argc, char **argv)
{
    assert(gMysqlPool);
    Thread th("test", thread);
    th.run();

    while (1) {
        MySqlConn *conn = gMysqlPool->GetAConnection();
        if (conn == nullptr) {
            LOGI("in main() conn is nullptr");
            sleep(1);
            continue;
        }
        if (conn->SelectSql("userinfo", nullptr, "user_name = \"alias\"") == OK) {
            SqlResBase::sp res = conn->getSqlRes();
            if (res->isVaild()) {
                int fields = res->getColumnCount();
                LOGI("fields = %d", fields);
                for (int i = 0; i < fields; ++i) {
                    cout << conn->getFieldByIdx(i);
                    cout << "\t";
                }
                cout << endl;
                while (res->next()) {
                    for (int i = 0; i < fields; ++i) {
                        cout << res->getString(i) << "\t";
                    }
                    cout << endl;
                }
            } else {
                LOGE("Unable to get resource");
            }
        }
        gMysqlPool->FreeAConnection(conn);
        sleep(1);
    }
    return 0;
}
