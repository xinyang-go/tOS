//
// Created by xinyang on 2020/6/22.
//

#ifndef TOS_OBJMANAGER_H
#define TOS_OBJMANAGER_H

#include "../tOS_config.h"
#include <fmt/format.h>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace tOS {
    struct AnyObj {
        void *any;
        std::atomic_size_t *ref;
    };

    struct MtxMap {
        std::mutex mtx;
        std::unordered_map<std::string, AnyObj> map;
    };

    enum class ObjType {
        MESSAGE = 0, REQUEST, NODE, LOGGER, USR_OBJ, TYPE_NUM
    };

    const std::unordered_map<ObjType, std::string> obj_type_name = {
            {ObjType::MESSAGE, "MESSAGE"},
            {ObjType::REQUEST, "REQUEST"},
            {ObjType::NODE,    "NODE"},
            {ObjType::LOGGER,  "LOGGER"},
            {ObjType::USR_OBJ, "USR_OBJ"}
    };

    inline MtxMap obj_map[static_cast<int>(ObjType::TYPE_NUM)];

    enum class OpenMode {
        FIND = 0, CREATE, FIND_OR_CREATE, MODE_NUM
    };

    struct empty_shared_obj_error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct shared_obj_type_error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    template<class T, bool CHECK = TOS_CHECK_DEFAULT>
    class SharedObj {
    private:
        static SharedObj find(ObjType type, const std::string &name) {
            if constexpr(CHECK)
                if (type < static_cast<ObjType>(0) || type >= ObjType::TYPE_NUM)
                    throw shared_obj_type_error(fmt::format("no such object type: {}", type));
            std::unique_lock lock(obj_map[static_cast<int>(type)].mtx);
            auto &map = obj_map[static_cast<int>(type)].map;
            auto iter = map.find(name);
            if (iter == map.end()) return SharedObj();
            return SharedObj(type, iter);
        }

        template<class ...Ts>
        static SharedObj create(ObjType type, const std::string &name, Ts &&...args) {
            if constexpr(CHECK)
                if (type < static_cast<ObjType>(0) || type >= ObjType::TYPE_NUM)
                    throw shared_obj_type_error(fmt::format("no such object type: {}", type));
            std::unique_lock lock(obj_map[static_cast<int>(type)].mtx);
            auto &map = obj_map[static_cast<int>(type)].map;
            auto iter = map.find(name);
            if (iter != map.end()) return SharedObj();
            auto *any = new T{std::forward<Ts>(args)...};
            auto *ref = new std::atomic_size_t(0);
            auto[new_iter, success] = map.emplace(name, AnyObj{any, ref});
            if (new_iter == map.end() || !success) {
                delete any;
                delete ref;
                return SharedObj();
            }
            return SharedObj(type, new_iter);
        }

        template<class ...Ts>
        static SharedObj find_or_create(ObjType type, const std::string &name, Ts &&...args) {
            if constexpr(CHECK)
                if (type < static_cast<ObjType>(0) || type >= ObjType::TYPE_NUM)
                    throw shared_obj_type_error(fmt::format("no such object type: {}", type));
            std::unique_lock lock(obj_map[static_cast<int>(type)].mtx);
            auto &map = obj_map[static_cast<int>(type)].map;
            auto iter = map.find(name);
            if (iter != map.end()) return SharedObj(type, iter);
            auto *any = new T{std::forward<Ts>(args)...};
            auto *ref = new std::atomic_size_t(0);
            auto[new_iter, success] = map.emplace(name, AnyObj{any, ref});
            if (new_iter == map.end() || !success) {
                delete any;
                delete ref;
                return SharedObj();
            }
            return SharedObj(type, new_iter);
        }

    public:

        template<OpenMode MODE, class ...Ts>
        static SharedObj make(ObjType type, const std::string &name, Ts &&...args) {
            static_assert(MODE == OpenMode::FIND || MODE == OpenMode::CREATE || MODE == OpenMode::FIND_OR_CREATE);
            if constexpr(MODE == OpenMode::FIND) {
                return find(type, name, std::forward<Ts>(args)...);
            } else if constexpr(MODE == OpenMode::CREATE) {
                return create(type, name, std::forward<Ts>(args)...);
            } else if constexpr(MODE == OpenMode::FIND_OR_CREATE) {
                return find_or_create(type, name, std::forward<Ts>(args)...);
            }
        }

    private:
        using iterator = std::unordered_map<std::string, AnyObj>::iterator;

        ObjType type;
        iterator iter;
    public:
        using value_type = T;

        ~SharedObj() { reset(); }

        SharedObj(ObjType t = ObjType::TYPE_NUM, const iterator &i = obj_map[0].map.end()) : type(t), iter(i) {
            if (*this) (*iter->second.ref)++;
        }

        SharedObj(const SharedObj &o) : type(o.type), iter(o.iter) {
            if (*this) (*iter->second.ref)++;
        }

        SharedObj(SharedObj &&o) : type(o.type), iter(o.iter) {
            o.type = ObjType::TYPE_NUM;
            o.iter = obj_map[0].map.end();
        }

        SharedObj &operator=(const SharedObj &o) {
            type = o.type;
            iter = o.iter;
            if (*this) (*iter->second.ref)++;
            return *this;
        }

        SharedObj &operator=(SharedObj &&o) {
            type = o.type;
            iter = o.iter;
            o.type = ObjType::TYPE_NUM;
            o.iter = obj_map[0].map.end();
            return *this;
        }

        operator bool() const {
            return type != ObjType::TYPE_NUM && iter != obj_map[0].map.end();
        }

        void reset() {
            if (!*this) return;
            std::unique_lock lock(obj_map[static_cast<int>(type)].mtx);
            if (--(*iter->second.ref) == 0) {
                delete static_cast<T *>(iter->second.any);
                delete iter->second.ref;
                obj_map[static_cast<int>(type)].map.erase(iter);
            }
            type = ObjType::TYPE_NUM;
            iter = obj_map[0].map.end();
        }

        T &operator*() const {
            if constexpr(CHECK) if (!*this) throw empty_shared_obj_error("try access empty shared object.");
            return *static_cast<T *>(iter->second.any);
        }

        T *operator->() const {
            if constexpr(CHECK) if (!*this) throw empty_shared_obj_error("try access empty shared object.");
            return static_cast<T *>(iter->second.any);
        }
    };
}

#endif /* TOS_OBJMANAGER_H */
