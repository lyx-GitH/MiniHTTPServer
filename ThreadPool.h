//
// Created by 刘宇轩 on 2022/11/11.
//

#ifndef MYHTTPSERVER_THREADPOOL_H
#define MYHTTPSERVER_THREADPOOL_H

#include <iostream>
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <utility>

#include "ThreadSafeQueue.h"


class ThreadPool {
    using TaskFuncType = std::function<void()>;
public:
    explicit ThreadPool(int num) : threads_(num), stop_(false) {}

    void run() {
        for (auto &i: threads_) {
            i = std::thread(&ThreadPool::running, this);
        }
    }

    void running() {
        while (!stop_) {
            TaskFuncType wf;
            if (!queue_.empty()) {
                queue_.pop(wf);
                try {
                    wf();
                } catch (std::bad_function_call &) {
                    std::cerr << "bad function call" << std::endl;
                }

            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }


    template<typename F, typename... Args>
    auto addTask(F &&func, Args &&...args) -> std::future<decltype(func(args...))> {
        using ret_type = decltype(func(args...));

        std::function<ret_type()> bind_func = std::bind(std::forward<F>(func), std::forward<Args>(args)...);

        auto task = std::make_shared<std::packaged_task<ret_type()>>(bind_func);
        TaskFuncType task_func = [task]() {
            (*task)();
        };

        queue_.push(task_func);
        cond_var_.notify_one();
        return task->get_future();
    }

    template<class T, typename F, typename... Args>
    auto addTask(T *class_, F &&p_mem_func, Args &&...args) -> std::future<decltype((class_->*p_mem_func)(args...))> {
        using ret_type = decltype((class_->*p_mem_func)(args...));

        std::function<ret_type()> bind_func = std::bind(p_mem_func, class_, std::forward<Args>(args)...);

        auto task = std::make_shared<std::packaged_task<ret_type()>>(bind_func);
        TaskFuncType wt = [task]() {
            (*task)();
        };

        queue_.push(wt);
        cond_var_.notify_one();
        return task->get_future();
    }


    void stop() {
        stop_ = true;
        cond_var_.notify_all();
    }

    ~ThreadPool() {
        std::cout << "ThreadPool is quiting..." << std::endl;
        for (auto &thread: threads_) {
            thread.join();
        }
        std::cout << "ThreadPool ends." << std::endl;
    }

private:
    ThreadSafeQueue<TaskFuncType> queue_;
    std::vector<std::thread> threads_;
    std::atomic<bool> stop_;
    std::condition_variable cond_var_;
};


#endif //MYHTTPSERVER_THREADPOOL_H
