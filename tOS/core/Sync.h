//
// Created by xinyang on 2020/8/31.
//

#ifndef TOS_SYNC_H
#define TOS_SYNC_H

#include <condition_variable>
#include <mutex>

namespace tOS {

    /*
     * 用于多任务同步
     * 典型的condition_variable的应用
     * 用于某个任务等待某个特定的标志
     */
    template<class T>
    class Sync {
        T val;
        std::mutex mtx;
        std::condition_variable cv;

    public:
        template<class ...Ts>
        explicit Sync(Ts &&... objs) : val(std::forward<Ts>(objs)...) {}

        void update(const T &v) {
            std::unique_lock lock(mtx);
            if (v != val) {
                val = v;
                cv.notify_all();
            }
        }

        void update(T &&v) {
            std::unique_lock lock(mtx);
            if (v != val) {
                val = std::move(v);
                cv.notify_all();
            }
        }

        void wait(const T &v) {
            std::unique_lock lock(mtx);
            cv.wait(lock, [&]() { return val == v; });
        }
    };
}

#endif //TOS_SYNC_H
