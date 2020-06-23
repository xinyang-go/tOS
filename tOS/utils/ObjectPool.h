//
// Created by xinyang on 2020/6/2.
//

#ifndef TOS_OBJECTPOOL_H
#define TOS_OBJECTPOOL_H

#include "../tOS_config.h"
#include "CircularQueue.h"
#include <exception>
#include <memory>
#include <mutex>

namespace tOS {
    /* 含锁对象池
     * T: 对象池元素类型
     * size: 对象池容量
     * CHECK: 是否开启运行时错误检查
     */
    template<class T, std::size_t SIZE, bool CHECK = TOS_CHECK_DEFAULT>
    class ObjectPool {
    private:
        static_assert(SIZE > 0);
        T objs[SIZE];
        // 使用循环队列存储未被分配的对象地址。
        CircularQueue<T *, SIZE, false> available;
        std::mutex mtx;
    public:
        ObjectPool() { // 初始化时把所有对象地址加入队列。
            for (T *ptr = objs; ptr < objs + SIZE; ptr++) {
                available.push(ptr);
            }
        }

        // 不允许拷贝
        ObjectPool(const ObjectPool &) = delete;

        ObjectPool &operator=(const ObjectPool &) = delete;

        // 分配一个对象
        inline T *alloc() {
            if (available.size() == 0) throw std::bad_alloc();
            std::unique_lock lock(mtx);
            return available.pop();
        }

        // 释放一个对象
        inline void free(T *ptr) {
            if constexpr (CHECK) { // 检查参数的合法性。
                if (ptr < objs || objs + SIZE <= ptr || ((uintptr_t) ptr - (uintptr_t) objs) % sizeof(T) != 0) {
                    throw std::runtime_error("invalid free of ObjectPool!");
                }
            }
            std::unique_lock lock(mtx);
            available.push(ptr);
        }

        // 分配一个共享对象，必须保证该对象池的生命周期大于共享对象。
        inline std::shared_ptr<T> alloc_shared() {
            return std::shared_ptr<T>((std::remove_all_extents_t<T> *) alloc(), [this](void *ptr) { free((T *) ptr); });
        }
    };
}

#endif /* TOS_OBJECTPOOL_H */
