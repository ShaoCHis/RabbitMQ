#pragma once
#include <memory>
#include <thread>
#include <functional>

namespace RabbitMQ{
    class ThreadPool{
    public:
        ThreadPool();
        ~ThreadPool();
    std::function<void(int)> f;
    };
}

