#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "AsyncBuffer.hpp"

namespace mylog
{
    enum class AsyncType
    {
        ASYNC_SAFE,
        ASYNC_UNSAFE
    }; // 异步类型

    using functor = std::function<void(Buffer &)>; // 回调函数类型
    // using 用于 定义类型别名（类似 typedef）
    // std::function 是 C++11 引入的 可调用对象封装器，用于存储 函数指针、Lambda 表达式、仿函数（Functor）等可调用对象
    // std::function<void(Buffer &)> 代表一个 可调用对象类型

    class AsyncWorker
    {
    public:
        using ptr = std::shared_ptr<AsyncWorker>; // 智能指针类型

        AsyncWorker(const functor &cb, AsyncType async_type = AsyncType::ASYNC_SAFE)
            : async_type_(async_type),
              callback_(cb),
              stop_(false),
              thread_(std::thread(&AsyncWorker::ThreadEntry, this)) {}
        // 创建并启动一个新的线程，线程执行 AsyncWorker 类的 ThreadEntry 成员函数
        // this：绑定当前对象，使 ThreadEntry 在 this 指向的对象上执行
        ~AsyncWorker() { Stop(); }
        // 写入数据
        void Push(const char *data, size_t len)
        {
            // 如果生产者队列不足以写下len长度数据，并且缓冲区是固定大小，那么阻塞
            std::unique_lock<std::mutex> lock(mtx_);
            if (AsyncType::ASYNC_SAFE == async_type_) // 冲区固定大小（ASYNC_SAFE）
                cond_productor_.wait(lock, [&]()
                                     { return len <= buffer_productor_.WriteableSize(); });
            buffer_productor_.Push(data, len); // 写入数据
            cond_consumer_.notify_one();       // 通知消费者线程
        }
        // 停止
        void Stop()
        {
            stop_ = true;                // 设置停止标志
            cond_consumer_.notify_all(); // 所有线程把缓冲区内数据处理完就结束了
            thread_.join();              // 等待线程结束
        }

    private:
        // 线程入口
        void ThreadEntry()
        {
            while (1)
            {
                { // 缓冲区交换完就解锁，让productor继续写入数据
                    std::unique_lock<std::mutex> lock(mtx_);
                    if (buffer_productor_.IsEmpty() && stop_)
                        // 如果 buffer_productor_ 为空且 stop_ 为 false，阻塞等待
                        // 有数据则交换，无数据就阻塞
                        cond_consumer_.wait(lock, [&]()
                                            { return stop_ || !buffer_productor_.IsEmpty(); });
                    buffer_productor_.Swap(buffer_consumer_);
                    // 生产者缓冲区 buffer_productor_ 和消费者缓冲区 buffer_consumer_ 交换
                    // ，以便释放 buffer_productor_ 让 Push() 继续写入数据

                    // 固定容量的缓冲区才需要唤醒
                    if (async_type_ == AsyncType::ASYNC_SAFE)
                        cond_productor_.notify_one();
                }
                callback_(buffer_consumer_); // 调用回调函数对缓冲区中数据进行处理
                buffer_consumer_.Reset();
                if (stop_ && buffer_productor_.IsEmpty())
                    return;
            }
        }

    private:
        AsyncType async_type_;                   // 异步类型
        std::atomic<bool> stop_;                 // 用于控制异步工作器的启动
        std::mutex mtx_;                         // 互斥锁
        mylog::Buffer buffer_productor_;         // 生产者缓冲区
        mylog::Buffer buffer_consumer_;          // 消费者缓冲区
        std::condition_variable cond_productor_; // 生产者条件变量
        std::condition_variable cond_consumer_;  // 消费者条件变量
        std::thread thread_;                     // 线程

        functor callback_; // 回调函数，用来告知工作器如何落地
    };
} // namespace mylog