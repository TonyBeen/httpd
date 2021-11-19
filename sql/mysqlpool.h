/*************************************************************************
    > File Name: mysqlpool.h
    > Author: hsz
    > Brief:
    > Created Time: Wed 10 Nov 2021 09:47:53 AM CST
 ************************************************************************/

#ifndef __MYSQL_POOL_H__
#define __MYSQL_POOL_H__

#include <utils/string8.h>
#include <stdint.h>
#include <vector>

namespace Jarvis {
class MysqlPool
{
public:
    MysqlPool(uint32_t sqlHandleNum, const String8 &userName = "mysql",
              const String8 &passWord = "123456", const String8 &IP = "127.0.0.1", const uint16_t &port = 3306);
    ~MysqlPool();


private:

};

} // namespace Jarvis

#endif // __MYSQL_POOL_H__