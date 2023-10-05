#include "db.h"
#include <muduo/base/Logging.h>

// ���ݿ�������Ϣ
static string server = "127.0.0.1";
static string user = "root";
static string password = "root";
static string dbname = "chat";

// ��ʼ�����ݿ�����
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// �ͷ����ݿ�������Դ
// ������UserModelʾ��,ͨ��UserModel��ζ�ҵ����װ�ײ����ݿ�Ĳ�����
// ����ʾ�����£�
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// �������ݿ�
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    // c��c++����Ĭ�ϵı����ַ���ASCII,���������,��MySQL�����µ�������ʾ?
    if (p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql failure!";
    }
    return p;
}
// ���²���
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "Update Failed!";
        return false;
    }
    return true;
}
// ��ѯ����
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "Query Failed!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL *MySQL::getConnection()
{
    return _conn;
}
