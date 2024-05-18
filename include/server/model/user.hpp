#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// User表的ORM类,映射表的字段（是一个映射类，因为ORM就是对象关系映射）
class User // 对照数据库中的user表建立对应的类
{  
public:
    User(int id=-1, string name="", string pwd="", string state="offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }

protected:
    int id;
    string name;
    string password;
    string state;

};


#endif