#ifndef __GROUPUSER_HPP__
#define __GROUPUSER_HPP__

#include "user.hpp"

class GroupUser : public User
{
public:
    // 继承User的内容，并增加权限(角色)
    void setRole(string role) { _role = role; }
    string getRole() { return _role; }
private:
    string _role;
};

#endif