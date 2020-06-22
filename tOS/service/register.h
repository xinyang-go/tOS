//
// Created by xinyang on 2020/6/21.
//

#ifndef TOS_REGISTER_H
#define TOS_REGISTER_H

#include "../tOS.h"
#include <iostream>
#include <unordered_map>

namespace tOS::service {

    using Func_t = int (*)(int argc, const char *argv[]);
    using EntryFunc_t = Func_t;
    using CmdFunc_t = Func_t;

    using FuncMap_t = std::unordered_map<std::string, Func_t>;

    inline FuncMap_t cmd_map, entry_map;


    struct Register {
        Register(const char *name, FuncMap_t &map, Func_t func) {
            auto[iter, suc] = map.emplace(name, func);
            if (iter == map.end() || !suc) {
                std::cerr << "'" << name << "' register failed!" << std::endl;
            }
        }
    };

    struct PreMainExec {
        template<class Func_t>
        PreMainExec(Func_t func) { func(); }
    };
}

#define CMD_EXPORT(func)                __attribute__((unused)) tOS::service::Register \
                                        _register_cmd_##func##_(#func, tOS::service::cmd_map, func)
#define CMD_EXPORT_ALIAS(func, alias)   __attribute__((unused)) tOS::service::Register \
                                        _register_cmd_##alias##_(#alias, tOS::service::cmd_map, func)

#define ENTRY_EXPORT(func)              __attribute__((unused)) tOS::service::Register \
                                        _register_entry_##func##_(#func, tOS::service::entry_map, func)
#define ENTRY_EXPORT_ALIAS(func, alias) __attribute__((unused)) tOS::service::Register \
                                        _register_entry_##alias##_(#alias, tOS::service::entry_map, func)

#define EXEC_EXPORT(func)               __attribute__((unused)) tOS::service::PreMainExec _exec_##func##_(func)

#endif /* TOS_REGISTER_H */
