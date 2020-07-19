//
// Created by xinyang on 2020/6/3.
//

#ifndef TOS_REQUEST_H
#define TOS_REQUEST_H

#include "Container.h"
#include "Message.h"
#include <future>

namespace tOS {
    // 服务端
    template<class R>
    class Server;

    // 客户端
    template<class R>
    class Client;

    /* 请求包，默认请求容器满时会覆盖未处理的请求。
     * 即同一请求仅可被不同服务端中的某一位处理
     * S: 请求元素类型
     * B: 请求返回类型
     * size: 请求包容量
     * Container: 请求包容器，也即请求包传递方式。包括循环队列和栈
     */
    template<class S, class B, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
    class Request {
        static_assert(SIZE > 0, "size must larger than 1.");
        static_assert(Container == CIRCULAR_QUEUE || CIRCULAR_QUEUE == STACK,
                      "Container must be either CIRCULAR_QUEUE or STACK.");

        friend class Server<Request>;

        friend class Client<Request>;

    private:
        using SendType = S;
        using BackType = B;

        using T = std::pair<S, std::promise<B>>;
        using C = std::conditional_t<Container == CIRCULAR_QUEUE,
                CircularQueue < T, SIZE, false>, Stack<T, SIZE, false>>;

        // 所有服务端从同一个容器获取请求。
        C c;
        // 该请求上的服务段的个数
        std::size_t server_ref{0};
        // 该请求上的客户段的个数
        std::size_t client_ref{0};
        // 用于线程同步
        std::mutex mtx;
        std::condition_variable cv;

        void attach_client() {
            std::unique_lock lock(mtx);
            client_ref++;
        }

        void detach_client() {
            std::unique_lock lock(mtx);
            client_ref--;
        }

        void attach_server() {
            std::unique_lock lock(mtx);
            server_ref++;
        }

        void detach_server() {
            std::unique_lock lock(mtx);
            server_ref--;
        }

        std::future<B> push(const S &obj) {
            std::unique_lock lock(mtx);
            std::promise<B> promise;
            auto future = promise.get_future();
            if (c.full()) c.pop();
            c.emplace(obj, std::move(promise));
            cv.notify_all();
            return future;
        }

        std::future<B> push(S &&obj) {
            std::unique_lock lock(mtx);
            std::promise<B> promise;
            auto future = promise.get_future();
            if (c.full()) c.pop();
            c.emplace(std::move(obj), std::move(promise));
            cv.notify_all();
            return std::move(future);
        }

        template<class ...Ts>
        std::future<B> emplace(Ts &&...args) {
            std::unique_lock lock(mtx);
            std::promise<B> promise;
            auto future = promise.get_future();
            if (c.full()) c.pop();
            c.emplace(S{std::forward<Ts>(args)...}, std::move(promise));
            cv.notify_all();
            return std::move(future);
        }


        bool pop(T &obj) {
            std::unique_lock lock(mtx);
            cv.wait(lock, [this]() { return !c.empty(); });
            obj = std::move(c.pop());
            return true;
        }

        template<typename _Rep, typename _Period>
        bool pop(T &obj, const std::chrono::duration<_Rep, _Period> &dt) {
            std::unique_lock lock(mtx);
            if (!cv.wait_for(lock, dt, [this]() { return !c.empty(); })) return false;
            obj = std::move(c.pop());
            return true;
        }
    };

    // 用于判断一个类型是否为Request。
    template<class T>
    constexpr bool isRequest = false;

    template<class S, class B, std::size_t SIZE, ContainerEnum Container>
    constexpr bool isRequest<Request<S, B, SIZE, Container>> = true;

    /* 客户端
     * R: 请求包类型
     */
    template<class R>
    class Client {
        static_assert(isRequest<R>, "R must be Request.");
    private:
        SharedObj <R> r;

    public:
        using SendType = typename R::SendType;
        using BackType = typename R::BackType;

        ~Client() {
            reset();
        }

        Client() = default;

        Client(const SharedObj <R> &_r) : r(_r) { r->attach_server(); }

        Client(SharedObj <R> &&_r) : r(std::move(_r)) { r->attach_server(); }

        Client(const Client &p) = delete;

        Client(Client &&p) = default;

        Client &operator=(const Client &p) = delete;

        Client &operator=(Client &&p) = default;

        operator bool() { return r; }

        inline std::future<BackType> push(const SendType &obj) {
            return r->push(obj);
        }

        inline std::future<BackType> push(SendType &&obj) {
            return r->push(std::move(obj));
        }

        template<class ...Ts>
        inline std::future<BackType> emplace(Ts &&...args) {
            return r->emplace(std::forward<Ts>(args)...);
        }

        inline void reset() {
            if (!r) return;
            r->detach_server();
            r.reset();
        }

        inline std::size_t get_server_num() { return r->server_ref; }

        inline std::size_t get_client_num() { return r->client_ref; }
    };

    /* 服务端
     * R: 请求包类型
     */
    template<class R>
    class Server {
        static_assert(isRequest<R>, "R must be Request.");
    private:
        SharedObj <R> r;
    public:
        using SendType = typename R::SendType;
        using BackType = typename R::BackType;
        using T = typename R::T;

        ~Server() {
            reset();
        }

        Server() = default;

        Server(const SharedObj <R> &_r) : r(_r) { r->attach_client(); }

        Server(SharedObj <R> &&_r) : r(std::move(_r)) { r->attach_client(); }

        Server(const Server &p) = delete;

        Server(Server &&p) = default;

        Server &operator=(const Server &p) = delete;

        Server &operator=(Server &&p) = default;

        operator bool() { return r; }

        inline bool pop(T &obj) {
            return r->pop(obj);
        }

        template<typename _Rep, typename _Period>
        inline bool pop(T &obj, const std::chrono::duration<_Rep, _Period> &dt) {
            return r->pop(obj, dt);
        }

        inline void reset() {
            if (!r) return;
            r->detach_client();
            r.reset();
        }

        inline std::size_t get_server_num() { return r->server_ref; }

        inline std::size_t get_client_num() { return r->client_ref; }
    };
}

#endif /* TOS_REQUEST_H */
