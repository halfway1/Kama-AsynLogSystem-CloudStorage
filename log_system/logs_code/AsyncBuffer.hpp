/*日志缓冲区类设计*/
#pragma once
#include <cassert>
#include <string>
#include <vector>
#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    class Buffer
    {
    public:
        Buffer() : write_pos_(0), read_pos_(0)
        {
            buffer_.resize(g_conf_data->buffer_size);
        }
        // 写入数据
        void Push(const char *data, size_t len)
        {
            ToBeEnough(len); // 确保容量足够
            // 开始写入
            std::copy(data, data + len, &buffer_[write_pos_]);
            write_pos_ += len;
        }
        // 读取数据
        char *ReadBegin(int len)
        {
            assert(len <= ReadableSize());
            return &buffer_[read_pos_];
        }
        // 判断是否为空
        bool IsEmpty() { return write_pos_ == read_pos_; }
        // 交换两个缓冲区
        void Swap(Buffer &buf)
        {
            buffer_.swap(buf.buffer_);
            std::swap(read_pos_, buf.read_pos_);
            std::swap(write_pos_, buf.write_pos_);
        }
        // 写空间剩余容量
        size_t WriteableSize()
        {
            return buffer_.size() - write_pos_;
        }
        // 读空间剩余容量
        size_t ReadableSize()
        {
            return write_pos_ - read_pos_;
        }
        // 返回缓冲区起始位置
        const char *Begin() { return &buffer_[read_pos_]; }
        // 移动写位置
        void MoveWritePos(int len)
        {
            assert(len <= WriteableSize());
            write_pos_ += len;
        }
        // 移动读位置
        void MoveReadPos(int len)
        {
            assert(len <= ReadableSize());
            read_pos_ += len;
        }
        // 重置缓冲区
        void Reset()
        {
            write_pos_ = 0;
            read_pos_ = 0;
        }

    protected:
        // 确保容量足够
        void ToBeEnough(size_t len)
        {
            int buffersize = buffer_.size();
            // 如果写空间剩余容量小于len，则需要扩容
            if (len >= WriteableSize())
            {
                // 如果缓冲区大小小于阈值，则扩容
                if (buffer_.size() < g_conf_data->threshold)
                {
                    buffer_.resize(2 * buffer_.size() + buffersize);
                }
                // 如果缓冲区大小大于阈值，则线性扩容
                else
                {
                    buffer_.resize(g_conf_data->linear_growth + buffersize);
                }
            }
        }

    protected:
        std::vector<char> buffer_; // 缓冲区
        size_t write_pos_;         // 生产者此时的位置
        size_t read_pos_;          // 消费者此时的位置
    };
} // namespace mylog