#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
template <typename T>
class safeQueue : protected std::queue<T>
{
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lck(mutex);
            std::queue<T>::push(std::forward<T>(value));
        }
        std::unique_lock<std::mutex> lck(syncLock);
        consumer.notify_one();
    }
    bool pop(T& value) {
        std::lock_guard<std::mutex> lck(mutex);
        if (std::queue<T>::empty()) {
            return false;
        }
        else {
            value=std::move(std::queue<T>::front());
            std::queue<T>::pop();
        }
        return true;
    }
    void sync_pop(T &value) {
        if (!empty()) {
            pop(value);
            return;
        }
        std::unique_lock<std::mutex> lck(syncLock);
        consumer.wait(lck,[this](){
            return !empty();
        });
        pop(value);
        return;
    }
    bool empty() {
        std::lock_guard<std::mutex> lck(mutex);
        return std::queue<T>::empty();
    }
private:
    std::mutex mutex,syncLock;
    std::condition_variable consumer;
};


#endif