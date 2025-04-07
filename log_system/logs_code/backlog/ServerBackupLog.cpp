// 远程备份debug等级以上的日志信息-接收端
#include <string>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ServerBackupLog.hpp"
using std::cout;
using std::endl;
// cout 和 endl 直接使用，避免 std:: 前缀
const std::string filename = "./logfile.log";
// 该函数用于打印使用错误
void usage(std::string procgress)
{
    cout << "usage error:" << procgress << "port" << endl;
}
// 该函数用于检查文件是否存在
bool file_exist(const std::string &name)
{
    struct stat exist;
    return (stat(name.c_str(), &exist) == 0);
    // stat(name.c_str(), &exist) 获取文件状态，如果返回 0，说明文件存在
}
// 该函数用于备份日志
void backup_log(const std::string &message)//用作回调
{
    FILE *fp = fopen(filename.c_str(), "ab");
    if (fp == NULL)
    {
        perror("fopen error: ");
        assert(false);
    }
    int write_byte = fwrite(message.c_str(), 1, message.size(), fp);
    if (write_byte != message.size())
    {
        perror("fwrite error: ");
        assert(false);
    }
    fflush(fp); // 刷新缓冲区，将缓冲区的内容写入文件
    fclose(fp);
}
// 该函数用于启动备份服务器
int main(int args, char *argv[])
{
    if (args != 2)
    {
        usage(argv[0]);
        perror("usage error");
        exit(-1);
    }
    // 将字符串转换为整数
    uint16_t port = atoi(argv[1]);
    // 创建一个 TcpServer 对象，并使用 backup_log 作为回调函数
    std::unique_ptr<TcpServer> tcp(new TcpServer(port, backup_log));
    // 初始化服务
    tcp->init_service();
    // 启动服务
    tcp->start_service();
    // 返回 0
    return 0;
}