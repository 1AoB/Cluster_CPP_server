#include "chatservice.hpp"
#include <string>
#include "public.hpp"
#include "muduo/base/Logging.h"
#include <vector>
#include <map>

using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 处理登录业务 id pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int id = js["id"].get<int>(); ////////////////////////////////////////////////////////////////
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录,不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "The user has logged in, please enter a new account!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功,记录用户连接信息     (这个{}非常妙)进作用域于加锁,出了作用域自动解锁
            {
                lock_guard<mutex> lock(_connMutex); // std::lock_guard是一个RAII类，它在构造时获取互斥锁，并在析构时释放互斥锁。
                _userConnMap.insert({id, conn});
            }
            // 登录成功,更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后,把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friend"] = vec2;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在,登录失败
        // 或者 用户存在但是用户名或密码错误,登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "User's name or password error!";
        conn->send(response.dump());
    }
}
// 注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    LOG_INFO << "do reg service!!!";
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
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}
// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线,转发消息 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    // toid 不在线,存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}
// 添加好友业务
void ChatService::addFirend(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}
// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    string name = js["name"].get<string>();
    string desc = js["desc"].get<string>();

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
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
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 存储离线群消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}

//  获取消息对应的处理器
MsgHander ChatService::getHandler(int msgid)
{
    // 记录错误日志,msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器,是一个空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR
                << "Can not find handler:[" << msgid << "]!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}
// 服务器异常,业务重置方法
void ChatService::reset()
{
    // 把online状态的用户,设置成offline
    _userModel.resetState();
}

// 处理客户端异常退出( ^] quit)
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    LOG_INFO << "clientCloseException";
    User user;
    {
        lock_guard<mutex> lock(_connMutex); // 为了上临界区更小,我们将其放入到一个{}中
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 注册消息以及对应的Handler回调函数
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFirend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}
