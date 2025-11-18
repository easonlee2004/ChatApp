#include "usermodel.hpp"
#include "db.h"

// User表的增加方法
bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 将插入后生成的主键值给User
            user.setId(mysql_insert_id(mysql.getConnection())); // 这里mysql变量类型是MySQL自定义类型，getconnection返回的才是MYSQL类型（大小写）
            return true;
        }
    }

    return false;
}