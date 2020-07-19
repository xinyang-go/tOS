//
// Created by xinyang on 2020/6/2.
//

#ifndef TOS_CIRCULARQUEUE_H
#define TOS_CIRCULARQUEUE_H

#include "../tOS_config.h"
#include <stdexcept>

namespace tOS {
    /* 循环队列位置标志结构体
     * size：队列容量
     */
    template<std::size_t SIZE>
    class QueuePos {
    private:
        // 使用flip标志可以避免常规循环队列最大容量比数组长度少一
        std::size_t pos: sizeof(std::size_t) * 8 - 1;
        std::size_t flip: 1;
    public:
        QueuePos() : pos(0), flip(0) {};

        // 位置自增
        inline QueuePos &operator++() {
            if (++pos == SIZE) {
                pos = 0, flip = ~flip;
            }
            return *this;
        }

        inline const QueuePos operator++(int) {
            QueuePos tmp = *this;
            if (++pos == SIZE) {
                pos = 0, flip = ~flip;
            }
            return tmp;
        }

        // 取位置
        inline explicit operator std::size_t() const { return pos; }

        // 位置比较
        inline std::size_t operator-(const QueuePos<SIZE> &p2) const {
            return pos - p2.pos + (flip == p2.flip ? 0 : SIZE);
        }

        inline bool operator==(const QueuePos<SIZE> &p2) const {
            return pos == p2.pos && flip == p2.flip;
        }

        inline bool operator!=(const QueuePos<SIZE> &p2) const {
            return pos != p2.pos || flip != p2.flip;
        }
    };


    /* 无锁循环队列
     * T: 循环队列元素类型
     * size：循环队列容量
     * CHECK：是否开启运行时错误检查。
     */
    template<class T, std::size_t SIZE, bool CHECK = TOS_CHECK_DEFAULT>
    class CircularQueue {
    public:
        T buffer[SIZE];
        QueuePos<SIZE> head, tail;
    public:
        using ValType = T;

        CircularQueue() = default;

        inline std::size_t size() const { return tail - head; }

        inline bool empty() const { return tail - head == 0; }

        inline bool full() const { return tail - head == SIZE; }

        inline void push(const T &obj) {
            if constexpr (CHECK) if (full()) throw std::range_error("queue full!");
            buffer[static_cast<std::size_t>(tail++)] = obj;
        }

        inline void push(T &&obj) {
            if constexpr (CHECK) if (full()) throw std::range_error("queue full!");
            buffer[static_cast<std::size_t>(tail++)] = std::move(obj);
        }

        template<class ...Ts>
        inline void emplace(Ts &&... args) {
            if constexpr (CHECK) if (full()) throw std::range_error("queue full!");
            buffer[static_cast<std::size_t>(tail++)] = T{std::forward<Ts>(args)...};
        }

        inline T pop() {
            if constexpr (CHECK) if (empty()) throw std::range_error("queue empty!");
            return std::move(buffer[static_cast<std::size_t>(head++)]);
        }
    };

    // 用于判断一个类型是否为CircularQueue。
    template<class T>
    constexpr bool isCircularQueue = false;

    template<class T, std::size_t SIZE, bool CHECK>
    constexpr bool isCircularQueue<CircularQueue<T, SIZE, CHECK>> = true;
}

#endif /* TOS_CIRCULARQUEUE_H */
