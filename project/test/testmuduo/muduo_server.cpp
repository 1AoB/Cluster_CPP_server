/*����muduo����⿪��������
1.���TcpServer
2.����EventLoopʱ��ѭ�������ָ��
3.��ȷTcpServer���캯����Ҫʲô���������ChatServer�Ĺ��캯��
4.�ڵ�ǰ��������Ĺ��캯�����У���⴦�����ӵĻص������ͳ���д�¼��Ļص�����
5.���ú��ʵķ�����߳�������muduo����Լ�����IO�̺߳�worker�߳�
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
#include <functional>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // ʱ��ѭ��
               const InetAddress &listenAddr, // IP+port
               const string &nameArg)         // ������������
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // oop, listenAddr, nameArg��������ʼ������server��loop����ʼ��_loop
        // ��������ע���û����ӵĴ����ͶϿ��ص�
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // ��������ע���д�¼��ص�
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        // ���÷������˵��߳����� 1��IO�߳� 1��worker�߳�
        _server.setThreadNum(4);
    }

    // �����¼�ѭ��
    void start()
    {
        _server.start();
    }

private:
    // ר�Ŵ����û������Ӵ����ͶϿ�
    void onConnection(const TcpConnectionPtr &conn)
    {
        // epoll listenfd accept
        // ֻ��ע�ͻ��˵Ĵ����ͶϿ�
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort()
                 << "->" << conn->localAddress().toIpPort() << "state:online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort()
                 << "->" << conn->localAddress().toIpPort() << "state:offline" << endl;
            conn->shutdown(); // �൱��close(fd);
            //_loop->quit();//�˳�epoll
        }
    }
    // ר�Ŵ����û��Ķ�д�¼�
    void onMessage(const TcpConnectionPtr &conn, // ����
                   Buffer *buffer,               // ������
                   Timestamp time)               // ���յ����ݵ�ʱ����Ϣ
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << "time:" << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop *_loop;
};

int main()
{
    EventLoop loop; // �񴴽���epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // ��listenfd��epoll��_ctl��ӵ�epoll
    loop.loop();    // �������epoll_wait��������ʽ�ȴ����û����ӣ��������û��Ķ�д�¼���

    return 0;
}
// g++ -o server muduo_server.cpp -lmuduo_net -lmuduo_base -pthread
// telnet 127.0.0.1 6000

// ctrl+] Ȼ����telnet���������� quit �Ϳ����˳��ˡ�

/*************
 * cmake ���÷�:
 * ����build�ļ���:ִ�� make..
 * Ȼ����ִ�� make
 *
 *
 * ************/