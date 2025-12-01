#ifndef __GROUPMODEL_HPP__
#define __GROUPMODEL_HPP__

#include "group.hpp"

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 用户加入指定群组，并设置权限
    void addGroup(int userid, int groupid, string role);

    // 查询给定用户所在的所有群
    vector<Group> queryGroups(int userid);

    // 查询给定组内所有成员id(除去自己)
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif