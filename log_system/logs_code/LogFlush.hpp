#include <cassert>
#include <fstream>
#include <memory>
#include <unistd.h>
#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;
namespace mylog
{
    class LogFlush
    {
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush() {}
        virtual void Flush(const char *data, size_t len) = 0; // 不同的写文件方式Flush的实现不同
    };
    // 标准输出
    class StdoutFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<StdoutFlush>;
        void Flush(const char *data, size_t len) override
        {
            cout.write(data, len);
        }
    };
    // 文件输出
    class FileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<FileFlush>;

        FileFlush(const std::string &filename) : filename_(filename)
        {
            // 创建所给目录
            Util::File::CreateDirectory(Util::File::Path(filename));
            // 打开文件
            fs_ = fopen(filename.c_str(), "ab"); // a (append)：以追加模式 b (binary)：以二进制模式
            if (fs_ == NULL)
            {
                std::cout << __FILE__ << __LINE__ << "open log file failed" << std::endl;
                perror(NULL);
            }
        }
        // 写入文件
        void Flush(const char *data, size_t len) override
        {
            fwrite(data, 1, len, fs_); // 写入文件
            if (ferror(fs_))
            { // 如果写入失败
                std::cout << __FILE__ << __LINE__ << "write log file failed" << std::endl;
                perror(NULL);
            }
            if (g_conf_data->flush_log == 1)
            { // 如果配置为1
                if (fflush(fs_) == EOF)
                { // 刷新缓冲区
                    std::cout << __FILE__ << __LINE__ << "fflush file failed" << std::endl;
                    perror(NULL);
                }
            }
            else if (g_conf_data->flush_log == 2)
            {                       // 如果配置为2
                fflush(fs_);        // 刷新缓冲区
                fsync(fileno(fs_)); // 同步文件
            }
        }

    private:
        std::string filename_;
        FILE *fs_ = NULL;
    };

    class RollFileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<RollFileFlush>;

        RollFileFlush(const std::string &filename, size_t max_size)
            : max_size_(max_size), basename_(filename)
        {
            Util::File::CreateDirectory(Util::File::Path(filename));
        }
        // 当文件大小超过 max_size_ 时，创建新日志文件，并 关闭旧文件

        void Flush(const char *data, size_t len) override
        {
            // 确认文件大小不满足滚动需求
            InitLogFile();
            // 向文件写入内容
            fwrite(data, 1, len, fs_);
            // 如果写入失败
            if (ferror(fs_))
            {
                std::cout << __FILE__ << __LINE__ << "write log file failed" << std::endl;
                perror(NULL);
            }
            cur_size_ += len; // 更新当前文件大小

            if (g_conf_data->flush_log == 1)
            { // 如果配置为1
                if (fflush(fs_))
                { // 刷新缓冲区
                    std::cout << __FILE__ << __LINE__ << "fflush file failed" << std::endl;
                    perror(NULL);
                }
            }
            else if (g_conf_data->flush_log == 2)
            {                       // 如果配置为2
                fflush(fs_);        // 刷新缓冲区
                fsync(fileno(fs_)); // 同步文件
            }
        }

    private:
        // 初始化日志文件
        void InitLogFile()
        {
            if (fs_ == NULL || cur_size_ >= max_size_) // 如果文件为空或当前文件大小超过最大大小
            {
                if (fs_ != NULL)
                {                // 如果文件不为空
                    fclose(fs_); // 关闭文件
                    fs_ = NULL;
                }
                std::string filename = CreateFilename(); // 创建新日志文件
                fs_ = fopen(filename.c_str(), "ab");     // 打开新日志文件
                if (fs_ == NULL)
                { // 如果打开失败
                    std::cout << __FILE__ << __LINE__ << "open file failed" << std::endl;
                    perror(NULL);
                }
                cur_size_ = 0; // 重置当前文件大小
            }
        }

        // 构建落地的滚动日志文件名称
        std::string CreateFilename()
        {
            time_t time_ = Util::Date::Now();
            struct tm t;
            localtime_r(&time_, &t);
            std::string filename = basename_;
            filename += std::to_string(t.tm_year + 1900);
            filename += std::to_string(t.tm_mon + 1);
            filename += std::to_string(t.tm_mday);
            filename += std::to_string(t.tm_hour + 1);
            filename += std::to_string(t.tm_min + 1);
            filename += std::to_string(t.tm_sec + 1) + '-' +
                        std::to_string(cnt_++) + ".log";
            return filename;
        }

    private:
        size_t cnt_ = 1;       // 文件序号
        size_t cur_size_ = 0;  // 当前文件大小
        size_t max_size_;      // 最大文件大小
        std::string basename_; // 日志文件名
        FILE *fs_ = NULL;      // 文件指针
    };

    // LogFlushFactory 作为工厂类，用于创建不同的 LogFlush 实例
    class LogFlushFactory
    {
    public:
        using ptr = std::shared_ptr<LogFlushFactory>;
        // 创建日志输出对象
        template <typename FlushType, typename... Args>
        static std::shared_ptr<LogFlush> CreateLog(Args &&...args)
        {
            return std::make_shared<FlushType>(std::forward<Args>(args)...);
        }
    };
} // namespace mylog