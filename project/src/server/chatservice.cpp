#include "chatservice.hpp"
#include <string>
#include "public.hpp"
#include "muduo/base/Logging.h"
#include <vector>
#include <map>

using namespace muduo;
using namespace std;

// ��ȡ��������Ľӿں���
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// �����¼ҵ�� id pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int id = js["id"].get<int>(); ////////////////////////////////////////////////////////////////
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // ���û��Ѿ���¼,�������ظ���¼
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "The user has logged in, please enter a new account!";
            conn->send(response.dump());
        }
        else
        {
            // ��¼�ɹ�,��¼�û�������Ϣ     (���{}�ǳ���)���������ڼ���,�����������Զ�����
            {
                lock_guard<mutex> lock(_connMutex); // std::lock_guard��һ��RAII�࣬���ڹ���ʱ��ȡ����������������ʱ�ͷŻ�������
                _userConnMap.insert({id, conn});
            }
            // ��¼�ɹ�,�����û�״̬��Ϣ state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // ��ѯ���û��Ƿ���������Ϣ
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // ��ȡ���û���������Ϣ��,�Ѹ��û�������������Ϣɾ����
                _offlineMsgModel.remove(id);
            }
            // ��ѯ���û��ĺ�����Ϣ������
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
        // ���û�������,��¼ʧ��
        // ���� �û����ڵ����û������������,��¼ʧ��
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "User's name or password error!";
        conn->send(response.dump());
    }
}
// ע��ҵ�� name password
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
        // ע��ɹ�
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // ע��ʧ��
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}
// һ��һ����ҵ��
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid����,ת����Ϣ ����������������Ϣ��toid�û�
            it->second->send(js.dump());
            return;
        }
    }
    // toid ������,�洢������Ϣ
    _offlineMsgModel.insert(toid, js.dump());
}
// ��Ӻ���ҵ��
void ChatService::addFirend(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // �洢������Ϣ
    _friendModel.insert(userid, friendid);
}
// ����Ⱥ��ҵ��
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    string name = js["name"].get<string>();
    string desc = js["desc"].get<string>();

    // �洢�´�����Ⱥ����Ϣ
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // �洢Ⱥ�鴴������Ϣ
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// ����Ⱥ��ҵ��
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// Ⱥ������ҵ��
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
            // ת��Ⱥ��Ϣ
            it->second->send(js.dump());
        }
        else
        {
            // �洢����Ⱥ��Ϣ
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}

//  ��ȡ��Ϣ��Ӧ�Ĵ�����
MsgHander ChatService::getHandler(int msgid)
{
    // ��¼������־,msgidû�ж�Ӧ���¼�����ص�
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // ����һ��Ĭ�ϵĴ�����,��һ���ղ���
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
// �������쳣,ҵ�����÷���
void ChatService::reset()
{
    // ��online״̬���û�,���ó�offline
    _userModel.resetState();
}

// ����ͻ����쳣�˳�( ^] quit)
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    LOG_INFO << "clientCloseException";
    User user;
    {
        lock_guard<mutex> lock(_connMutex); // Ϊ�����ٽ�����С,���ǽ�����뵽һ��{}��
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // ��map��ɾ���û���������Ϣ
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // �����û���״̬��Ϣ
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// ע����Ϣ�Լ���Ӧ��Handler�ص�����
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFirend, this, _1, _2, _3)});

    // Ⱥ��ҵ���������¼�����ص�ע��
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}
