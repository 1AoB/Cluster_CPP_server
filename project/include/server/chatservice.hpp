#ifndef CHATSERVICE_H
#define CHATSERVICE_H
// 服务,业务
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include "usermodel.hpp"
#include "json.hpp"
#include "offlinemessagemodel.hpp"
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHander = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 单例模式
// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp);
    // 添加好友业务
    void addFirend(const TcpConnectionPtr &conn, json &js, Timestamp);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHander getHandler(int msgid);

    // 服务器异常,业务重置方法
    void reset();

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

private:
    ChatService(); // 构造函数私有化
    // 存储消息id和其对应的事件业务处理方法
    unordered_map<int, MsgHander> _msgHandlerMap; // 消息id对应的处理操作

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁,保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    // 存储离线消息
    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;
    GroupModel _groupModel;
};

#endif