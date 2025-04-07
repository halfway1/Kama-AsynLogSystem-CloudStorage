/* ************************************************************************
> File Name:     test.cpp
> Created Time:  Thu 07 Sep 2023 06:37:16 PM CST
> Description:
 ************************************************************************/
#define DEBUG_LOG
#include "Service.hpp"
#include <thread>
using namespace std;

storage::DataManager *data_;
ThreadPool *tp = nullptr;
mylog::Util::JsonData *g_conf_data;
// 该函数用于启动服务
void service_module()
{
    storage::Service s;
    mylog::GetLogger("asynclogger")->Info("service step in RunModule");
    s.RunModule();
}
// 该函数用于初始化日志系统
void log_system_module_init()
{
    g_conf_data = mylog::Util::JsonData::GetJsonData();                    // 获取 JSON 配置信息
    tp = new ThreadPool(g_conf_data->thread_count);                        // 创建线程池
    std::shared_ptr<mylog::LoggerBuilder> Glb(new mylog::LoggerBuilder()); // 创建日志构建器
    Glb->BuildLoggerName("asynclogger");                                   // 设置日志名称
    Glb->BuildLoggerFlush<mylog::RollFileFlush>("./logfile/RollFile_log",
                                                1024 * 1024); // 设置日志文件路径和大小
    // The LoggerManger has been built and is managed by members of the LoggerManger class
    // The logger is assigned to the managed object, and the caller lands the log by invoking the singleton managed object
    mylog::LoggerManager::GetInstance().AddLogger(Glb->Build()); // 添加日志
}

int main()
{
    log_system_module_init();
    data_ = new storage::DataManager();

    thread t1(service_module);

    t1.join();
    delete (tp);
    return 0;
}