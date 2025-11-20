#include "usermodel.hpp"
#include <mysql/mysql.h>
#include "db.h"

// User表的增加方法
bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 将插入后生成的主键值给User
            user.setId(mysql_insert_id(mysql.getConnection())); // 注意区别MySQL自定义类型和getconnection返回的MYSQL类型（大小写）
            return true;
        }
    }

    return false;
}

// 根据id查询用户
User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES * res = mysql.query(sql); // 查询结果
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                // row的0 1 2 3分别对应User表的四个字段
                User user;
                user.setId(atoi(row[0])); // 字符串转int
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                // 记得释放mysql res开辟的空间
                mysql_free_result(res);
                return user;
            }
        }
        mysql_free_result(res);
    }

    return User(); // 失败返回默认构造，id为-1
}