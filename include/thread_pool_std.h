#ifndef THREAD_POOL_STD_H
#define THREAD_POOL_STD_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <future>

// Standard library thread pool implementation for Windows compatibility
class ThreadPoolStd {
public:
    explicit ThreadPoolStd(size_t num_threads);
    ~ThreadPoolStd();

    // Submit a task to the thread pool
    template<typename F>
    void submit(F task);

    // Wait for all submitted tasks to complete
    void wait_for_all();

    // Get optimal number of threads for this system
    static size_t get_optimal_thread_count();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex tasks_mutex;
    std::condition_variable task_condition;
    std::condition_variable finished_condition;
    
    bool stop_pool;
    size_t busy_workers;

    void worker_thread();
};

template<typename F>
void ThreadPoolStd::submit(F task) {
    {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        if (stop_pool) {
            return;
        }
        tasks.push(std::function<void()>(task));
    }
    task_condition.notify_one();
}

#endif // THREAD_POOL_STD_H

