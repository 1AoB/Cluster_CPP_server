/*基于muduo网络库开发服务器
1.组合TcpServer
2.创建EventLoop时间循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，组测处理连接的回调函数和出读写事件的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配IO线程和worker线程
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
    ChatServer(EventLoop *loop,               // 时间循环
               const InetAddress &listenAddr, // IP+port
               const string &nameArg)         // 服务器的名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // oop, listenAddr, nameArg三者来初始化——server，loop来初始化_loop
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        // 设置服务器端的线程数量 1个IO线程 1个worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理用户的连接创建和断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        // epoll listenfd accept
        // 只关注客户端的创建和断开
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort()
                 << "->" << conn->localAddress().toIpPort() << "state:online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort()
                 << "->" << conn->localAddress().toIpPort() << "state:offline" << endl;
            conn->shutdown(); // 相当于close(fd);
            //_loop->quit();//退出epoll
        }
    }
    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 接收到数据的时间信息
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
    EventLoop loop; // 像创建了epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // 将listenfd用epoll—_ctl添加到epoll
    loop.loop();    // 像调用了epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}
// g++ -o server muduo_server.cpp -lmuduo_net -lmuduo_base -pthread
// telnet 127.0.0.1 6000

// ctrl+] 然后在telnet命令行输入 quit 就可以退出了。

/*************
 * cmake 的用法:
 * 进入build文件夹:执行 make..
 * 然后再执行 make
 *
 *
 * ************/