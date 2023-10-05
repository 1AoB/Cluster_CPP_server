#ifndef CHATSERVICE_H
#define CHATSERVICE_H
// ����,ҵ��
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

// ��ʾ������Ϣ���¼��ص���������
using MsgHander = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// ����ģʽ
// ���������ҵ����
class ChatService
{
public:
    // ��ȡ��������Ľӿں���
    static ChatService *instance();
    // �����¼ҵ��
    void login(const TcpConnectionPtr &conn, json &js, Timestamp);
    // ����ע��ҵ��
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp);
    // һ��һ����ҵ��
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp);
    // ��Ӻ���ҵ��
    void addFirend(const TcpConnectionPtr &conn, json &js, Timestamp);
    // ����Ⱥ��ҵ��
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp);
    // ����Ⱥ��ҵ��
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // Ⱥ������ҵ��
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // ��ȡ��Ϣ��Ӧ�Ĵ�����
    MsgHander getHandler(int msgid);

    // �������쳣,ҵ�����÷���
    void reset();

    // ����ͻ����쳣�˳�
    void clientCloseException(const TcpConnectionPtr &conn);

private:
    ChatService(); // ���캯��˽�л�
    // �洢��Ϣid�����Ӧ���¼�ҵ������
    unordered_map<int, MsgHander> _msgHandlerMap; // ��Ϣid��Ӧ�Ĵ������

    // �洢�����û���ͨ������
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // ���廥����,��֤_userConnMap���̰߳�ȫ
    mutex _connMutex;

    // ���ݲ��������
    UserModel _userModel;
    // �洢������Ϣ
    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;
    GroupModel _groupModel;
};

#endif