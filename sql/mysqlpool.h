/*************************************************************************
    > File Name: mysqlpool.h
    > Author: hsz
    > Brief:
    > Created Time: Wed 10 Nov 2021 09:47:53 AM CST
 ************************************************************************/

#ifndef __MYSQL_POOL_H__
#define __MYSQL_POOL_H__

#include "mysql.h"
#include <utils/mutex.h>
#include <utils/string8.h>
#include <stdint.h>
#include <set>
#include <vector>
#include <atomic>

namespace Jarvis {
class MysqlPool
{
public:
    MysqlPool(uint16_t sqlHandleNum, const String8 &db = "userdb", const String8 &userName = "mysql",
              const String8 &passWord = "123456", const String8 &IP = "127.0.0.1", const uint16_t &port = 3306);
    ~MysqlPool();

    MySqlConn *GetAConnection();
    void FreeAConnection(MySqlConn *);

protected:
    bool Init();
    void free();

private:
    uint16_t    mMysqlConnNum;
    String8     mDatabaesName;
    String8     mPassword;
    String8     mUsername;
    String8     mIP;
    uint16_t    mPort;

    Sem         mFreeSignal;
    Mutex       mSqlMutex;
    std::atomic<bool>       mNeedFree;
    std::atomic<uint16_t>   mSqlLeftNum;
    std::vector<std::pair<MySqlConn *, bool>> mMysqlPool;
};

} // namespace Jarvis

#endif // __MYSQL_POOL_H__