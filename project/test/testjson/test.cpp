#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func1() // json ���л�
{
    json js;
    // ����ʽ��ϣ�����ƣ������
    js["msg_type"] = 2;
    js["from"] = "guanyu";
    js["to"] = "zhangfei";
    js["msg"] = "hello world";
    cout << js << endl;

    string sendBuf = js.dump(); // ��json���ݶ������л�Ϊ�ַ���
    cout << sendBuf.c_str() << endl;
    return sendBuf;
}

int main()
{
    string recvBuf = func1();
    // json �����л�
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;

    return 0;
}