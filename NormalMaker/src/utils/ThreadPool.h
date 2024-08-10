#pragma once

#include <queue>
#include <future>

class ThreadPool
{
public:
    ThreadPool(size_t numThreads);

    template<class Func, class... Args>
    auto enqueue(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;

    ~ThreadPool();

    inline const size_t GetTaksCount() const { return tasks.size(); }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};
