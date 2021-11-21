/*************************************************************************
    > File Name: mysqlpool.cpp
    > Author: hsz
    > Brief:
    > Created Time: Wed 10 Nov 2021 09:47:56 AM CST
 ************************************************************************/

#include "mysqlpool.h"
#include <log/log.h>

#define LOG_TAG "mysqlpool"

namespace Jarvis {
MysqlPool::MysqlPool(uint16_t sqlHandleNum, const String8 &db, const String8 &userName,
                     const String8 &passWord, const String8 &IP, const uint16_t &port) :
    mUsername(userName),
    mPassword(passWord),
    mIP(IP),
    mPort(port),
    mMysqlConnNum(sqlHandleNum),
    mDatabaesName(db),
    mFreeSignal(0),
    mNeedFree(false)
{
    mSqlLeftNum.store(sqlHandleNum);
    mMysqlPool.resize(sqlHandleNum);
    Init();
}

MysqlPool::~MysqlPool()
{
    free();
}

bool MysqlPool::Init()
{
    MySqlConn *conn = nullptr;
    for (int i = 0; i < mMysqlConnNum; ++i) {
        conn = new MySqlConn(mUsername.c_str(), mPassword.c_str(), mDatabaesName.c_str(), mIP.c_str(), mPort);
        if (conn == nullptr) {
            i--;
            continue;
        }
        if (conn->isSqlValid()) {
            mMysqlPool.push_back(std::make_pair(conn, true));
        } else {
            delete conn;
            mMysqlPool.push_back(std::make_pair(nullptr, true));
        }
    }
    return true;
}

void MysqlPool::free()
{
    mNeedFree = true;
    if (mSqlLeftNum.load() != mMysqlConnNum) { // 需要等所有mysqlconn释放后才能真正的释放资源
        mFreeSignal.wait();
    }
    for (auto it : mMysqlPool) {
        if (it.second == true) {
            delete it.first;
        }
    }
}

MySqlConn *MysqlPool::GetAConnection()
{
    if (mNeedFree.load() || mSqlLeftNum.load() == 0) {
        return nullptr;
    }
    LOGD("%s()", __func__);
    AutoLock<Mutex> lock(mSqlMutex);
    for (auto &it : mMysqlPool) {
        if (it.second) {
            LOGD("find a unused");
            if (it.first == nullptr) {
                it.first = new MySqlConn(mUsername.c_str(), mPassword.c_str(),
                                         mDatabaesName.c_str(), mIP.c_str(), mPort);
            }
            if (it.first->isSqlValid()) {
                --mSqlLeftNum;
                it.second = false;
                return it.first;
            } else {
                delete it.first;
                it.first = nullptr;
            }
        }
    }
    return nullptr;
}

void MysqlPool::FreeAConnection(MySqlConn *conn)
{
    LOGD("%s()", __func__);
    if (mNeedFree.load() && mSqlLeftNum.load() != mMysqlConnNum) {
        mFreeSignal.post();
    }

    AutoLock<Mutex> lock(mSqlMutex);
    for (auto &it : mMysqlPool) {
        if (it.first == conn) {
            LOGD("%s() find ptr: %p\n", __func__, conn);
            it.second = true;
            ++mSqlLeftNum;
        }
    }
}


} // namespace Jarvis
