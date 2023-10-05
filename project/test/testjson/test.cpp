#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func1() // json 序列化
{
    json js;
    // 与链式哈希表类似，无序的
    js["msg_type"] = 2;
    js["from"] = "guanyu";
    js["to"] = "zhangfei";
    js["msg"] = "hello world";
    cout << js << endl;

    string sendBuf = js.dump(); // 将json数据对象序列化为字符串
    cout << sendBuf.c_str() << endl;
    return sendBuf;
}

int main()
{
    string recvBuf = func1();
    // json 反序列化
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;

    return 0;
}