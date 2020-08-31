//
// Created by xinyang on 2020/6/3.
//

#ifndef TOS_NODE_H
#define TOS_NODE_H

#include <fmt/format.h>
#include "Sync.h"
#include "Message.h"
#include "Request.h"
#include "Logger.h"
#include "Container.h"
#include "ObjManager.h"

namespace tOS {
    // 线程id到node名称的map。
    inline std::unordered_map<std::thread::id, std::string> global_node_map;

    class Node {
        friend SharedObj<Node>;
    private:
        using ObjLogger = SharedObj<Logger>;

        const std::string name;

        // 私有构造使得该类不能被直接创建。
        // 必须在node线程内创建node对象。
        explicit Node(std::string n) : name(std::move(n)) {
            global_node_map[std::this_thread::get_id()] = name;
        };
    public:
        bool running{true};

        ~Node() {
            global_node_map.erase(std::this_thread::get_id());
        }

        template<OpenMode MODE, class T, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
        auto make_publisher(const std::string &message_name) const {
            return Publisher{SharedObj<MultiMessage<T, SIZE, Container>>
                             ::template make<MODE>(ObjType::MESSAGE, message_name)};
        }

        template<OpenMode MODE, class T, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
        auto make_subscriber(const std::string &message_name) const {
            return Subscriber{SharedObj<MultiMessage<T, SIZE, Container>>::template make<MODE>(
                    ObjType::MESSAGE, message_name)};
        }

        template<OpenMode MODE, class S, class B, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
        auto make_client(const std::string &request_name) const {
            return Client{SharedObj<Request<S, B, SIZE, Container>>::template make<MODE>(
                    ObjType::REQUEST, request_name)};
        }

        template<OpenMode MODE, class S, class B, std::size_t SIZE = 1, ContainerEnum Container = CIRCULAR_QUEUE>
        auto make_server(const std::string &request_name) const {
            return Server{SharedObj<Request<S, B, SIZE, Container>>::template make<MODE>(
                    ObjType::REQUEST, request_name)};
        }

        template<OpenMode MODE, class T>
        auto make_sync(const std::string &sync_name) const {
            return SharedObj<Sync<T>>::template make<MODE>(ObjType::SYNC, sync_name);
        }

        template<OpenMode MODE, class T, class ...Ts>
        SharedObj<T> make_object(const std::string &obj_name, Ts &&...args) const {
            return SharedObj<T>::template make<MODE>(
                    ObjType::USR_OBJ, obj_name, std::forward<Ts>(args)...);
        }

        SharedObj<Logger> make_logger() const {
            return SharedObj<Logger>::template make<OpenMode::FIND_OR_CREATE>(ObjType::LOGGER, name, name);
        }


        inline static SharedObj<Node> create_node(const std::string &name) {
            static std::atomic_size_t id = 0;
            auto n = fmt::format("{}-{}", name, id++);
            return SharedObj<Node>::template make<OpenMode::CREATE>(ObjType::NODE, n, n);
        }

        inline static SharedObj<Node> this_node() {
            return SharedObj<Node>::template make<OpenMode::FIND>(
                    ObjType::NODE, global_node_map[std::this_thread::get_id()]);
        }
    };
}

#endif /* TOS_NODE_H */
