#ifndef DB_H
#define DB_H

/**********
 *
 * ����ļ��ǹ̶���,ֻҪ�õ�mysql,�Ϳ�����!!!
 *
 *
 *
 * *************/
#include <mysql/mysql.h>
#include <string>

using namespace std;

// ���ݿ������
class MySQL
{
public:
    // ��ʼ�����ݿ�����
    MySQL();
    // �ͷ����ݿ�������Դ
    // ������UserModelʾ��,ͨ��UserModel��ζ�ҵ����װ�ײ����ݿ�Ĳ�����
    // ����ʾ�����£�
    ~MySQL();
    // �������ݿ�
    bool connect();
    // ���²���
    bool update(string sql);
    // ��ѯ����
    MYSQL_RES *query(string sql);
    // ��ȡ����
    MYSQL *getConnection();

private:
    MYSQL *_conn;
};

#endif
