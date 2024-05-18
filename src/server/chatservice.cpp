#include "chatservice.hpp"
#include "public.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <muduo/base/Logging.h>
using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户设置成offline
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志,msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid); // 不要用中括号查询，如果msgid不在，会先添加一对，这样属于起反作用
    if (it == _msgHandlerMap.end())       // 没找到
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else // 找到了
    {
        return _msgHandlerMap[msgid];
    }
}

// ORM 对象关系映射 业务层操作的都是对象  业务代码不能出现数据库的增删改查，业务层操作的都是对象
// 处理登录业务 用户输入id pwd，查看pwd正不正确
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>(); // js["id"]是string，使用.get<int>()转成int
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK; // 登录响应消息
            response["errno"] = 2;             // 错误代码为2
            response["errmsg"] = "This account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {                                       // 临界区代码段
                lock_guard<mutex> lock(_connMutex); // 在获得锁的同时进行锁的管理
                _userConnMap.insert({id, conn});
            } // 加这个大括号是让锁仅在这个括住的作用域里面生效，出了作用域自动解锁了(std::lock_guard 对象会自动被销毁，从而释放互斥量)
              // 因为后面没必要对mql的操作也加锁，mysql的线程安全由mysql服务器保证
            
            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息，state offline->online
            user.setState("online");
            _userModel.updateState(user); // 用户状态刷新

            json response;
            response["msgid"] = LOGIN_MSG_ACK; // 登录响应消息
            response["errno"] = 0;             // 错误代码，因为响应消息不一定是正确消息，检测errno==0说明没错，响应成功了
            response["id"] = user.getId();     // 用户的id
            response["name"] = user.getName();
            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在/用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK; // 登录响应消息
        response["errno"] = 1;             // 错误代码为1
        response["errmsg"] = "Userid or password wrong!";
        conn->send(response.dump());
    }
}

// 处理注册业务  用户来注册只需要填 name 和 password 两个字段
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK; // 注册响应消息
        response["errno"] = 0;           // 错误代码，因为响应消息不一定是正确消息，检测errno==0说明没错，响应成功了
        response["id"] = user.getId();   // 用户的id
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK; // 注册响应消息
        response["errno"] = 1;           // 错误代码，因为响应消息不一定是正确消息，检测errno==1说明有错，注册失败了，后面id啥的都不用看了
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "offline");
    _userModel.updateState(user);
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    /*  从_userConnMap中找到conn对应的id，然后从该map表中删除这个键值对，再将用户的state改成offline  */
    
    User user;
    {
        lock_guard<mutex> lock(_connMutex); // 修改_userConnMap需要注意线程安全
        // for循环找到conn匹配的那一项
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表中删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if(user.getId() != -1)  // 万一没找到这个id，就不用再去向数据库发送请求了
    {
        user.setState("offline");
        _userModel.updateState(user);
    }

}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();  // toid是要发送给那个用户的id

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            // toid 在线，转发消息   服务器主动推送消息给toid用户 (相当于两个id是在同一台服务器上的，直接转发即可)
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")  // 在线，说明不在和user同一台的另一台主机上
    {
        _redis.publish(toid, js.dump());  // 那就需要把聊天消息直接发布到toid通道上，那么以toid注册过的通道上就有事件发生了，就会给业务层上报了
        return;
    }

    // toid 不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());

}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid); 
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1,name, desc);
    if (_groupModel.createGroup(group))  // 创建成功(因为群组的名字是unique的，重复创建同名群组是不成功的)
    {
        // 存储群组创始人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump()); // 在id通道上发布消息，只管发布消息就行，群组成员所在的主机就能收到订阅的消息
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息, 这一句主要是为了应对在从消息队列取消息的过程中，接收消息的用户下线了，那么将存储离线消息
    _offlineMsgModel.insert(userid, msg);

}