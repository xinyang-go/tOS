//
// Created by xinyang on 2020/6/4.
//

#include "register.h"
#include <CLI/CLI.hpp>
#include <tabulate/table.hpp>
#include <pipes/pipes.hpp>
#include <fmt/format.h>
#include <thread>
#include <filesystem>

using namespace tOS;
using namespace tOS::service;

// 通配符匹配
static bool str_match(const char *str, const char *pattern) {
    while (*pattern != 0) {
        if (*str == 0) return false; // 无字符可匹配，匹配失败
        switch (*pattern) {
            case '?': // ?匹配单个任意字符
                str++, pattern++;
                break;
            case '*': // *匹配任意个任意字符
                while (*pattern == '*') pattern++; // 合并多个*
                if (*pattern == 0) return true; // *后没有字符，必定匹配成功
                while (*str != 0 && *str != *pattern) str++;
                break;
            default: // 匹配当前字符
                if (*str++ != *pattern++) return false;
        }
    }
    // 通配符结束，被匹配字符也应该结束，否则匹配失败
    return *str == 0;
}

int list(int argc, const char *argv[]) {
    bool show_entry = false, show_cmd = false, show_obj = false;
    CLI::App app("list");
    app.add_flag("-e,--entry", show_entry, "show the registered node entry.");
    app.add_flag("-c,--cmd", show_cmd, "show the registered shell command.");
    app.add_flag("-o,--obj", show_obj, "show the named object.");
    CLI11_PARSE(app, argc, argv);

    if (show_entry) {
        tabulate::Table table;
        table.add_row({"node entry", "address"})[0].format()
                .font_align(tabulate::FontAlign::center)
                .font_background_color(tabulate::Color::magenta);
        for (auto &[n, f]:entry_map) {
            table.add_row({n, fmt::format("{}", (void *) f)});
        }
        std::cout << table << std::endl;
    }
    if (show_cmd) {
        tabulate::Table table;
        table.add_row({"shell command", "address"})[0].format()
                .font_align(tabulate::FontAlign::center)
                .font_background_color(tabulate::Color::cyan);
        for (auto &[n, f]:cmd_map) {
            table.add_row({n, fmt::format("{}", (void *) f)});
        }
        std::cout << table << std::endl;
    }
    if (show_obj) {
        tabulate::Table table;
        table.add_row({"object type", "object name", "reference"})[0].format()
                .font_align(tabulate::FontAlign::center)
                .font_background_color(tabulate::Color::blue);
        for (int t = 0; t < static_cast<int>(ObjType::TYPE_NUM); t++) {
            for (auto &[n, v]: obj_map[t].map) {
                table.add_row({obj_type_name.at(static_cast<ObjType>(t)), n, fmt::format("{}", *v.ref)});
            }
        }
        std::cout << table << std::endl;
    }
    return 0;
}

CMD_EXPORT(list);

int exec(int argc, const char *argv[]) {
    std::string entry;
    std::vector<std::string> v_argv;
    CLI::App app("exec");
    app.add_option("entry", entry, "the entry to run.")->required();
    app.add_option("argv", v_argv, "the argv pass to entry.");
    CLI11_PARSE(app, argc, argv);

    auto iter = entry_map.find(entry);
    if (iter == entry_map.end()) {
        std::cerr << "entry '" << entry << "' not found." << std::endl;
        return -1;
    }

    std::thread([func = iter->second](std::string entry_, std::vector<std::string> v_argv_) {
        int argc_ = v_argv_.size() > TOS_MAX_TOKEN ? TOS_MAX_TOKEN : v_argv_.size();
        const char *argv_[TOS_MAX_TOKEN];
        for (int i = 0; i < argc_; i++) {
            argv_[i] = v_argv_[i].c_str();
        }
        auto node = Node::create_node(entry_);
        func(argc_, argv_);
    }, std::move(entry), std::move(v_argv)).detach();

    return 0;
}

CMD_EXPORT(exec);

int logger(int argc, const char *argv[]) {
    std::string node;
    LogLevel level;
    CLI::App app("logger");
    app.add_option("node", node,
                   "the node(s) to set log level.\n"
                   "leave empty for all nodes(including nodes create later).");
    app.add_option("-l,--level", level, "the log level.")
            ->required()
            ->transform(CLI::CheckedTransformer(
                    std::map<std::string, int>{{"none",   0},
                                               {"error",  1},
                                               {"waring", 2},
                                               {"info",   3}}));
    CLI11_PARSE(app, argc, argv);

    if (node.empty()) {
        global_log_level = level;
    } else {
        obj_map[static_cast<int>(ObjType::LOGGER)].map
                >>= pipes::filter([&](auto &p) { return str_match(p.first.c_str(), node.c_str()); }) // 根据通配符筛选
                >>= pipes::transform([](auto &p) { return static_cast<Logger *>(p.second.any); }) // 提取logger指针
                >>= pipes::for_each([=](auto *l) { l->local_log_level = level; }); // 设置level
    }
    return 0;
}

CMD_EXPORT(logger);


int stop(int argc, const char *argv[]) {
    std::string node;
    CLI::App app("logger");
    app.add_option("node", node, "the node(s) to stop.\n")->required();
    CLI11_PARSE(app, argc, argv);

    obj_map[static_cast<int>(ObjType::NODE)].map
            >>= pipes::filter([&](auto &p) { return str_match(p.first.c_str(), node.c_str()); }) // 根据通配符筛选
            >>= pipes::transform([](auto &p) { return static_cast<Node *>(p.second.any); }) // 提取node指针
            >>= pipes::for_each([](auto *n) { n->running = false; }); // 设置node运行状态

    return 0;
}

CMD_EXPORT(stop);

// 将输入流重定向到终端
int console(int argc, const char *argv[]) {
#ifdef __linux__
    if (freopen("/dev/tty", "r", stdin) == nullptr) {
        std::cerr << "resume to console fail!" << std::endl;
        return -1;
    }
#elif defined(__WIN32__)
    if (freopen("CON", "r", stdin) == nullptr) {
        std::cerr << "resume to console fail!" << std::endl;
        return -1;
    }
#else
    std::cerr << "unknown plantform. do not known how to switch to console." << std::endl;
    return -1;
#endif
    return 0;
}

CMD_EXPORT(console);

// 从script运行
int script(int argc, const char *argv[]) {
    std::string file;
    CLI::App app("script");
    app.add_option("file", file, "the script file to run.")->required();
    CLI11_PARSE(app, argc, argv);
    if (!std::filesystem::exists(file)) {
        std::cerr << file << ": No such file or directory" << std::endl;
        return -1;
    }
    if (freopen(file.c_str(), "r", stdin) == nullptr) {
        std::cerr << "can not open script file '" << "'." << std::endl;
        return -1;
    }
    return 0;
}

CMD_EXPORT(script);
