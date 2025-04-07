#include<unordered_map>
#include"AsyncLogger.hpp"

namespace mylog{
    // 通过单例对象对日志器进行管理，懒汉模式
    class LoggerManager
    {
    public:
        // 获取单例对象
        static LoggerManager &GetInstance()
        {
            static LoggerManager eton;
            return eton;
        }
        // 检查日志器是否存在
        bool LoggerExist(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mtx_);
            auto it = loggers_.find(name);
            if (it == loggers_.end())
                return false;
            return true;
        }
        // 添加日志器
        void AddLogger(const AsyncLogger::ptr &&AsyncLogger)
        {
            if (LoggerExist(AsyncLogger->Name()))
                return;
            std::unique_lock<std::mutex> lock(mtx_);
            loggers_.insert(std::make_pair(AsyncLogger->Name(), AsyncLogger));
        }
        // 获取日志器
        AsyncLogger::ptr GetLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mtx_);
            auto it = loggers_.find(name);
            if (it == loggers_.end())
                return AsyncLogger::ptr();
            return it->second;
        }
        // 获取默认日志器
        AsyncLogger::ptr DefaultLogger() { return default_logger_; }

    private:
        // 构造函数 
        LoggerManager()
        {
            std::unique_ptr<LoggerBuilder> builder(new LoggerBuilder()); // 创建日志器构建器
            builder->BuildLoggerName("default"); // 设置日志器名称      
            default_logger_ = builder->Build(); // 构建日志器   
            loggers_.insert(std::make_pair("default", default_logger_)); // 插入默认日志器  
        }

    private:
        std::mutex mtx_;
        AsyncLogger::ptr default_logger_;                              // 默认日志器
        std::unordered_map<std::string, AsyncLogger::ptr> loggers_; // 存放日志器
    };
}