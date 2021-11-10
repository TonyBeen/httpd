/*************************************************************************
    > File Name: sql_conn.h
    > Author: hsz
    > Mail:
    > Created Time: Fri 20 Aug 2021 08:57:27 AM CST
 ************************************************************************/

#ifndef __ALIAS_SQL_CONN_H__
#define __ALIAS_SQL_CONN_H__

#include "sql_base.h"
#include <utils/utils.h>
#include <map>

namespace Jarvis {
class MySqlConn : public SqlBase
{
public:
    DISALLOW_COPY_AND_ASSIGN(MySqlConn);
    MySqlConn(const char *sqlUser, const char *passwd, const char *db,
            const char *ip = "127.0.0.1", uint16_t port = 3306);
    ~MySqlConn();

    int  ConnectSql(const char *sqlUser, const char *passwd, const char *db,
                    const char *ip = "127.0.0.1", uint16_t port = 3306) override;

    int  SelectSql(const char *table, const char *value = nullptr, const char *cond = nullptr) override;
    int  InsertSql(const char *table, const char *value) override;
    int  UpdateSql(const char *table, const char *value, const char *cond) override;
    int  DeleteSql(const char *table, const char *cond) override;
    int  SqlCommond(const char *sql);

    int64_t         getAffectedRows() const;
    SqlResBase::sp  getSqlRes() override;
    uint32_t        getErrno() override { return mysql_errno(mSqlHandle); }
    const char *    getErrorStr() override { return mysql_error(mSqlHandle); }
    std::string     getFieldByIdx(uint32_t idx) const;

    bool isSqlValid() const { return mSqlInit; }
    void CloseConn();
    operator MYSQL*() { return mSqlHandle; }

protected:
    bool KeepField(const char *table, const char *value) override;
    bool KeepFieldByQuery(const char *table);

private:
    MYSQL *                     mSqlHandle;
    bool                        mSqlInit;
    std::map<int, std::string>  mFieldMap;      // select时查询的表字段及与位置的映射
};

class MySqlRes : public SqlResBase {
public:
    MySqlRes(MYSQL_RES* res, int eno, const char* estr);

    bool    isVaild() override { return mIsVaild; }
    bool    next() override;
    int     getErrno() const { return mErrno; }
    const std::string& getErrStr() const { return mErrStr; }
    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;

    bool            isValid(int idx);
    int8_t          getInt8(int idx) override;
    uint8_t         getUint8(int idx) override;
    int16_t         getInt16(int idx) override;
    uint16_t        getUint16(int idx) override;
    int32_t         getInt32(int idx) override;
    uint32_t        getUint32(int idx) override;
    int64_t         getInt64(int idx) override;
    uint64_t        getUint64(int idx) override;
    float           getFloat(int idx) override;
    double          getDouble(int idx) override;
    std::string     getString(int idx) override;
    time_t          getTime(int idx) override;

private:
    uint32_t        mErrno;
    std::string     mErrStr;
    bool            mIsVaild;
    MYSQL_ROW       mRow;
    unsigned long*  mRowLength;
    std::shared_ptr<MYSQL_RES> mResData;
};

} // namespace Jarvis

#endif // __ALIAS_SQL_CONN_H__