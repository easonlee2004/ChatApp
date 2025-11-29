#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values(%s, %s)",
        group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 将插入后生成的主键值给Group
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 用户加入指定群组，并设置权限
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, %s)", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询给定用户所在的所有群
vector<Group> GroupModel::queryGroup(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a \
        inner join groupuser b on a.id = b.userid where b.userid = %d", userid);

    vector<Group> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES * res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
        mysql_free_result(res);
    }

    // 查询并填写每个group中的users
    for (Group &group : vec)
    {
        char sql[1024] = {0};
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a \
        inner join groupuser b on a.id = b.userid where b.groupid = %d", group.getId());

        MYSQL_RES * res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user); // getUsers()需要返回引用类型才能pushback
            }
            mysql_free_result(res);
        }
        mysql_free_result(res);
    }

    return vec;
}

// 查询给定组内所有成员id(除去自己)
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES * res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
        mysql_free_result(res);
    }

    return vec;
}