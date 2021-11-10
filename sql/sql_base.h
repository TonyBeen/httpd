/*************************************************************************
    > File Name: sql_base.h
    > Author: hsz
    > Mail:
    > Created Time: Mon 23 Aug 2021 05:19:15 PM CST
 ************************************************************************/

#ifndef __ALIAS_SQL_BASE_H__
#define __ALIAS_SQL_BASE_H__

#include <mysql/mysql.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <memory>

namespace Jarvis {
class SqlResBase;
class SqlBase {
public:
    typedef std::shared_ptr<SqlBase> sp;
    SqlBase() {}
    virtual ~SqlBase() {}

    virtual int  ConnectSql(const char *sqlUser, const char *passwd, const char *db,
                const char *ip, uint16_t port) = 0;

    virtual int  SelectSql(const char *table, const char *value, const char *cond) = 0;
    virtual int  InsertSql(const char *table, const char *value) = 0;
    virtual int  UpdateSql(const char *table, const char *value, const char *cond) = 0;
    virtual int  DeleteSql(const char *table, const char *cond) = 0;

    virtual std::shared_ptr<SqlResBase>  getSqlRes() = 0;
    virtual uint32_t        getErrno() = 0; 
    virtual const char *    getErrorStr() = 0;
    virtual void CloseConn() = 0;

protected:
    virtual bool KeepField(const char *table, const char *value) { return true; }
};

class SqlResBase {
public:
    typedef std::shared_ptr<SqlResBase> sp;
    SqlResBase() {}
    virtual ~SqlResBase() {}

    virtual bool    isVaild() = 0;
    virtual int     getErrno() const = 0;
    virtual const   std::string& getErrStr() const = 0;

    virtual int getDataCount() = 0;
    virtual int getColumnCount() = 0;
    virtual int getColumnBytes(int idx) = 0;
    virtual int getColumnType(int idx) = 0;
    virtual std::string getColumnName(int idx) = 0;

    virtual int8_t getInt8(int idx) = 0;
    virtual uint8_t getUint8(int idx) = 0;
    virtual int16_t getInt16(int idx) = 0;
    virtual uint16_t getUint16(int idx) = 0;
    virtual int32_t getInt32(int idx) = 0;
    virtual uint32_t getUint32(int idx) = 0;
    virtual int64_t getInt64(int idx) = 0;
    virtual uint64_t getUint64(int idx) = 0;
    virtual float getFloat(int idx) = 0;
    virtual double getDouble(int idx) = 0;
    virtual std::string getString(int idx) = 0;
    virtual time_t getTime(int idx) = 0;
    virtual bool next() = 0;
};

} // namespace Jarvis

#endif // __ALIAS_SQL_BASE_H__