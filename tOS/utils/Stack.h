//
// Created by xinyang on 2020/6/2.
//

#ifndef TOS_STACK_H
#define TOS_STACK_H

#include <cstdint>
#include <stdexcept>

namespace tOS {
    /* 无锁栈
     * T: 栈元素类型
     * size: 栈容量
     * CHECK: 是否开启运行时错误检查
     */
    template<class T, std::size_t SIZE, bool CHECK = TOS_CHECK_DEFAULT>
    class Stack {
    private:
        T buffer[SIZE];
        std::size_t top;
    public:
        using ValType = T;

        Stack() : top(0) {};

        inline std::size_t size() const { return top; }

        inline bool empty() const { return top == 0; }

        inline bool full() const { return top == SIZE; }

        inline void push(const T &obj) {
            if constexpr(CHECK) if (full()) throw std::range_error("queue full!");
            buffer[top++] = obj;
        }

        inline void push(T &&obj) {
            if constexpr(CHECK) if (full()) throw std::range_error("queue full!");
            buffer[top++] = std::move(obj);
        }

        template<class ...Ts>
        inline void emplace(Ts &&... args) {
            if constexpr(CHECK) if (full()) throw std::range_error("queue full!");
            buffer[top++] = T(std::forward<Ts>(args)...);
        }

        inline T pop() {
            if constexpr(CHECK) if (empty()) throw std::range_error("queue empty!");
            return std::move(buffer[--top]);
        }
    };

    // 用于判断一个类型是否为Stack。
    template<class T>
    constexpr bool isStack = false;

    template<class T, std::size_t SIZE, bool CHECK>
    constexpr bool isStack<Stack<T, SIZE, CHECK>> = true;
}

#endif /* TOS_STACK_H */
