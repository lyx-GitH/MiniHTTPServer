//
// Created by 刘宇轩 on 2022/11/11.
//

#ifndef MYHTTPSERVER_THREADSAFEQUEUE_H
#define MYHTTPSERVER_THREADSAFEQUEUE_H

#include <queue>
#include <mutex>

template<typename T>
class ThreadSafeQueue {
    using lock_guard = std::lock_guard<std::mutex>;
public:
    void push(const T &e) {
        lock_guard lockGuard(mut_);
        safe_queue_.push(e);
    }

    void pop(T &e) {
        lock_guard lockGuard(mut_);
        if (!safe_queue_.empty()) {
            e = std::move(safe_queue_.front());
            safe_queue_.pop();
        }
    }

    bool empty() {
        return safe_queue_.empty();
    }

    __attribute__((unused)) size_t size() {
        return safe_queue_.size();
    }


private:
    std::queue<T> safe_queue_;
    std::mutex mut_;

};

#endif //MYHTTPSERVER_THREADSAFEQUEUE_H
