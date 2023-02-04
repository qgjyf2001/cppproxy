#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
//namespace  {
    template <typename T>
    class safeQueue {
    private:
        struct Node {
            Node* next;
            T val;
        };
    public:
        safeQueue() {
            Node *node=new Node();
            node->next=nullptr;
            head=node;
            tail=node;
        }
        void push_(T value) {
            auto node=new Node();
            node->val=std::move(value);
            node->next=nullptr;
            tail->next=node;
            tail=node;
        }
        void push(T value) {
            T _;
            push_(std::move(value));
            push_(std::move(_));
            if (notify) {
                notify=false;
                std::unique_lock<std::mutex> lck(syncLock);
                consumer.notify_one();
            }
        }
        T pop_() {
            auto removed=head;
            auto value=std::move(removed->val);
            head=removed->next;
            delete removed;
            return value;
        }
        T& front() {
            return head->next->val;
        }
        bool pop(T &value) {
            if (empty()) {
                return false;
            }
            pop_();
            value=pop_();
            return true;
        }
        void sync_pop(T &value) {
            if (!empty()) {
                pop(value);
                return;
            }
            notify=true;
            std::unique_lock<std::mutex> lck(syncLock);
            consumer.wait(lck,[this](){
                return !empty();
            });
            pop(value);
            return;
        }
        bool empty() {
            return head->next==nullptr||head->next->next==nullptr;
        }
        void clear() {
            while (!empty()) {
                T _;
                pop(_);
            }
        }
        ~safeQueue() {
            clear();
            delete head;
        }
    private:
        bool notify=false;
        std::mutex mutex,syncLock;
        std::condition_variable consumer;
        Node* head;
        Node* tail;
        
    };
//}

#endif