//
// Created by xinyang on 2020/6/3.
//

#ifndef TOS_MESSAGE_H
#define TOS_MESSAGE_H

#include "../utils/CircularQueue.h"
#include "../utils/Stack.h"
#include "Container.h"
#include "ObjManager.h"
#include <list>
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace tOS {
    // 消息发布器
    template<class M>
    class Publisher;

    // 消息订阅器
    template<class M>
    class Subscriber;

    enum class MessageStatus {
        OK = 0,     // 正常
        TIMEOUT,    // 超时
        EMPTY       // 消息无发布者
    };

    /* 单出口消息，默认消息容器满时会覆盖未取走的数据
     * 即同一消息仅可被不同订阅者中的某一位获取
     * T: 消息元素类型
     * size: 消息容量
     * Container: 消息容器，也即消息传递方式。包括循环队列和栈
     */
    template<class T, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
    class SingleMessage {
        static_assert(SIZE > 0, "size must larger than 1.");
        static_assert(Container == CIRCULAR_QUEUE || CIRCULAR_QUEUE == STACK,
                      "Container must be either CIRCULAR_QUEUE or STACK.");
        friend Publisher<SingleMessage>;
        friend Subscriber<SingleMessage>;
        friend SharedObj<SingleMessage>;
    private:
        using ValType = T;
        using C = std::conditional_t<Container == CIRCULAR_QUEUE,
                CircularQueue<T, SIZE, false>, Stack<T, SIZE, false>>;
        // 所有接收者对应同一个容器。
        C c;
        // 该消息上的发布者的个数
        std::size_t publisher_ref{0};
        // 该消息上的订阅者的个数
        std::size_t subscriber_ref{0};
        // 用于线程同步
        std::mutex mtx;
        std::condition_variable cv;

        using s_iter = Empty;
        using p_iter = Empty;

        // 私有构造使得该类不能被直接创建。
        SingleMessage() = default;

        p_iter attach_publisher() {
            std::unique_lock lock(mtx);
            publisher_ref++;
            return Empty();
        }

        void detach_publisher(const p_iter &iter) {
            std::unique_lock lock(mtx);
            if (--publisher_ref == 0) cv.notify_all();
        }

        s_iter attach_subscriber() {
            std::unique_lock lock(mtx);
            subscriber_ref++;
            return Empty();
        }

        void detach_subscriber(const s_iter &iter) {
            std::unique_lock lock(mtx);
            subscriber_ref--;
        }

        void push(const p_iter &iter, const T &obj) {
            std::unique_lock lock(mtx);
            if (c.full()) c.pop();
            c.push(obj);
            cv.notify_one();
        }

        void push(const p_iter &iter, T &&obj) {
            std::unique_lock lock(mtx);
            if (c.full()) c.pop();
            c.push(std::move(obj));
            cv.notify_one();
        }

        template<class ...Ts>
        void emplace(const p_iter &iter, Ts &&...args) {
            std::unique_lock lock(mtx);
            if (c.full()) c.pop();
            c.emplace(std::forward<Ts>(args)...);
            cv.notify_one();
        }

        MessageStatus pop(const s_iter &iter, T &obj) {
            std::unique_lock lock(mtx);
            cv.wait(lock, [this]() { return !c.empty() || publisher_ref == 0; });
            if (publisher_ref == 0) return MessageStatus::EMPTY;
            obj = std::move(c.pop());
            return MessageStatus::OK;
        }

        template<typename _Rep, typename _Period>
        MessageStatus pop(const s_iter &iter, T &obj, const std::chrono::duration<_Rep, _Period> &dt) {
            std::unique_lock lock(mtx);
            if (!cv.wait_for(lock, [this]() { return !c.empty() || publisher_ref == 0; }, dt))
                return MessageStatus::TIMEOUT;
            if (publisher_ref == 0) return MessageStatus::EMPTY;
            obj = std::move(c.pop());
            return MessageStatus::OK;
        }
    };

    // 用于判断一个类型是否为SingleMessage。
    template<class T>
    constexpr bool isSingleMessage = false;

    template<class T, std::size_t SIZE, ContainerEnum Container>
    constexpr bool isSingleMessage<SingleMessage<T, SIZE, Container>> = true;

    /* 多出口消息，默认消息容器满时会覆盖未取走的数据
     * 即同一消息可以被不同订阅者同时获取
     * T: 消息元素类型
     * size: 消息容量。
     * Container: 消息容器，也即消息传递方式。包括循环队列和栈
     */
    template<class T, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
    class MultiMessage {
        static_assert(SIZE > 0, "size must larger than 1.");
        static_assert(Container == CIRCULAR_QUEUE || CIRCULAR_QUEUE == STACK,
                      "Container must be either CIRCULAR_QUEUE or STACK.");
        friend Publisher<MultiMessage>;
        friend Subscriber<MultiMessage>;
        friend SharedObj<MultiMessage>;
    private:
        using ValType = T;
        using C = std::conditional_t<Container == CIRCULAR_QUEUE,
                CircularQueue<T, SIZE, false>, Stack<T, SIZE, false>>;
        // 每个接收者单独对应一个容器。
        std::list<C> cs;
        // 该消息上的发布者的个数
        std::size_t publisher_ref{0};
        // 该消息上的订阅者的个数
        std::size_t subscriber_ref{0};
        // 用于线程同步
        std::mutex mtx;
        std::condition_variable cv;

        using s_iter = typename std::list<C>::iterator;
        using p_iter = Empty;

        // 私有构造使得该类不能被直接创建。
        MultiMessage() = default;

        p_iter attach_publisher() {
            std::unique_lock lock(mtx);
            publisher_ref++;
            return Empty();
        }

        void detach_publisher(const p_iter &iter) {
            std::unique_lock lock(mtx);
            if (--publisher_ref == 0) cv.notify_all();
        }

        s_iter attach_subscriber() {
            std::unique_lock lock(mtx);
            subscriber_ref++;
            cs.emplace_front();
            return cs.begin();
        }

        void detach_subscriber(const s_iter &iter) {
            std::unique_lock lock(mtx);
            cs.erase(iter);
            subscriber_ref--;
        }

        void push(const p_iter &iter, const T &obj) {
            std::unique_lock lock(mtx);
            for (auto &v: cs) {
                if (v.full()) v.pop();
                v.push(obj);
            }
            cv.notify_all();
        }

        void push(const p_iter &iter, T &&obj) {
            std::unique_lock lock(mtx);
            for (auto &v: cs) {
                if (v.full()) v.pop();
                v.push(std::move(obj));
            }
            cv.notify_all();
        }

        template<class ...Ts>
        void emplace(const p_iter &iter, Ts &&...args) {
            std::unique_lock lock(mtx);
            for (auto &c: cs) {
                if (c.full()) c.pop();
                c.emplace(std::forward<Ts>(args)...);
            }
            cv.notify_all();
        }

        MessageStatus pop(const s_iter &iter, T &obj) {
            std::unique_lock lock(mtx);
            cv.wait(lock, [this, &iter]() { return !iter->empty() || publisher_ref == 0; });
            if (publisher_ref == 0) return MessageStatus::EMPTY;
            obj = std::move(iter->pop());
            return MessageStatus::OK;
        }

        template<typename _Rep, typename _Period>
        MessageStatus pop(const s_iter &iter, T &obj, const std::chrono::duration<_Rep, _Period> &dt) {
            std::unique_lock lock(mtx);
            if (!cv.wait_for(lock, dt, [this, &iter]() { return !iter->empty() || publisher_ref == 0; }))
                return MessageStatus::TIMEOUT;
            if (publisher_ref == 0) return MessageStatus::EMPTY;
            obj = std::move(iter->pop());
            return MessageStatus::OK;
        }
    };

    // 用于判断一个类型是否为MultiMessage。
    template<class T>
    constexpr bool isMultiMessage = false;

    template<class T, std::size_t SIZE, ContainerEnum Container>
    constexpr bool isMultiMessage<MultiMessage<T, SIZE, Container>> = true;

    /* 消息发布器
     * M: 消息类型
     */
    template<class M>
    class Publisher {
        static_assert(isMultiMessage<M> || isSingleMessage<M>, "M must be either SingleMessage or MultiMessage.");
    private:
        SharedObj<M> m;
        typename M::p_iter iter;

        using ValType = typename M::ValType;
    public:
        ~Publisher() {
            reset();
        }

        Publisher(const SharedObj<M> &_m) : m(_m), iter(m->attach_publisher()) {}

        Publisher(SharedObj<M> &&_m) : m(std::move(_m)), iter(m->attach_publisher()) {}

        Publisher(const Publisher &p) = delete;

        Publisher(Publisher &&p) = default;

        Publisher &operator=(const Publisher &p) = delete;

        Publisher &operator=(Publisher &&p) = default;

        operator bool() { return m; }

        inline void push(const ValType &obj) {
            m->push(iter, obj);
        }

        inline void push(ValType &&obj) {
            m->push(iter, std::move(obj));
        }

        template<class ...Ts>
        inline void emplace(Ts &&... args) {
            m->emplace(iter, std::forward<Ts>(args)...);
        }

        inline void reset() {
            if (!m) return;
            m->detach_publisher(iter);
            m.reset();
        }

        inline std::size_t get_publisher_num() { return m->publisher_ref; }

        inline std::size_t get_subscriber_num() { return m->subscriber_ref; }
    };

    /* 消息监听器
     * M: 消息类型
     */
    template<class M>
    class Subscriber {
        static_assert(isMultiMessage<M> || isSingleMessage<M>, "M must be either SingleMessage or MultiMessage.");
    private:
        SharedObj<M> m;
        typename M::s_iter iter;

        using ValType = typename M::ValType;
    public:
        ~Subscriber() {
            reset();
        }

        Subscriber() = default;

        Subscriber(const SharedObj<M> &_m) : m(_m), iter(m->attach_subscriber()) {}

        Subscriber(SharedObj<M> &&_m) : m(std::move(_m)), iter(m->attach_subscriber()) {}

        Subscriber(const Subscriber &p) = delete;

        Subscriber(Subscriber &&p) = default;

        Subscriber &operator=(const Subscriber &p) = delete;

        Subscriber &operator=(Subscriber &&p) = default;

        operator bool() { return m; }

        template<typename _Rep, typename _Period>
        inline MessageStatus pop(ValType &obj, const std::chrono::duration<_Rep, _Period> &dt) {
            return m->pop(iter, obj, dt);
        }

        inline MessageStatus pop(ValType &obj) {
            return m->pop(iter, obj);
        }

        inline void reset() {
            if (!m) return;
            m->detach_subscriber(iter);
            m.reset();
        }

        inline std::size_t get_publisher_num() { return m->publisher_ref; }

        inline std::size_t get_subscriber_num() { return m->subscriber_ref; }
    };
}

#endif /* TOS_MESSAGE_H */
