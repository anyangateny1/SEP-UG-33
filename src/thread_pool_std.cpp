#include "thread_pool_std.h"
#include <algorithm>

ThreadPoolStd::ThreadPoolStd(size_t num_threads) 
    : stop_pool(false), busy_workers(0) {
    
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back(&ThreadPoolStd::worker_thread, this);
    }
}

ThreadPoolStd::~ThreadPoolStd() {
    {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        stop_pool = true;
    }
    
    task_condition.notify_all();
    
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPoolStd::wait_for_all() {
    std::unique_lock<std::mutex> lock(tasks_mutex);
    finished_condition.wait(lock, [this] {
        return tasks.empty() && busy_workers == 0;
    });
}

size_t ThreadPoolStd::get_optimal_thread_count() {
    size_t hardware_threads = std::thread::hardware_concurrency();
    // Conservative threading for test execution environments (MAPTEK TITAN)
    // Cap at 4 threads to avoid overwhelming shared test infrastructure
    return std::max(static_cast<size_t>(1), std::min(hardware_threads, static_cast<size_t>(4)));
}

void ThreadPoolStd::worker_thread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(tasks_mutex);
            
            task_condition.wait(lock, [this] {
                return stop_pool || !tasks.empty();
            });
            
            if (stop_pool && tasks.empty()) {
                break;
            }
            
            task = tasks.front();
            tasks.pop();
            ++busy_workers;
        }
        
        // Execute the task
        task();
        
        {
            std::lock_guard<std::mutex> lock(tasks_mutex);
            --busy_workers;
        }
        
        finished_condition.notify_all();
    }
}
