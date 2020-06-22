//
// Created by xinyang on 2020/6/4.
//

#include "register.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <pipes/pipes.hpp>
#include <CLI/CLI.hpp>
#include <sstream>
#include <filesystem>

#ifdef WITH_BOOST_STACKTRACE

#include <boost/stacktrace.hpp>

#endif


using namespace tOS::service;

// 用于readline库的自动补全
static char *command_generator(const char *text, int state) {
    static int len;
    static auto p = cmd_map.begin();
    char *result = nullptr;
    // 第一次查找
    if (!state) {
        p = cmd_map.begin();
        len = strlen(text);
    }
    // 遍历命令列表
    for (; p != cmd_map.end(); ++p) {
        if (strncmp(p->first.c_str(), text, len) == 0) {
            result = strdup(p->first.c_str());
            ++p;
            break;
        }
    }
    // 无名称匹配，找到NULL
    return result;
}

// 用于readline库的自动补全
static char **fileman_completion(const char *str, int start, int end) {
    char **p_matches = nullptr;
    if (start == 0) {
        // 只补全第一个单词
        p_matches = rl_completion_matches(str, command_generator);
    }
    return p_matches;
}

// 执行一行命令
static void exec(char *str) {
    std::istringstream iss(str);
    std::vector<std::string> v_argv;
    int argc = 0;
    const char *argv[TOS_MAX_TOKEN];
    iss >>= pipes::read_in_stream<std::string>() >>= pipes::push_back(v_argv);
    if (v_argv.empty() || v_argv[0][0] == '#') return; // '#'开头为注释
    v_argv >>= pipes::for_each([&](auto &s) { argv[argc++] = s.c_str(); });
    auto iter = cmd_map.find(argv[0]);
    if (iter == cmd_map.end()) {
        std::cerr << "command '" << argv[0] << "' not found!" << std::endl;
        return;
    }
    iter->second(argc, argv);
}

int main(int argc, char *argv[]) {
    CLI::App app("tOS");
#ifdef WITH_BOOST_STACKTRACE
    std::string dump;
    app.add_option("-v,--vdump", dump, "the dump file to see.");
#endif
    std::string script;
    app.add_option("-s,--script", script, "the script file to run.");
    CLI11_PARSE(app, argc, argv);
#ifdef WITH_BOOST_STACKTRACE
    if (!dump.empty()) {
        if (!std::filesystem::exists(dump)) {
            std::cerr << dump << ": No such file or directory" << std::endl;
            return -1;
        }
        std::ifstream ifs(dump);
        if (!ifs) {
            std::cerr << "can not open dump file '" << "'." << std::endl;
            return -1;
        }
        auto st = boost::stacktrace::stacktrace::from_dump(ifs);
        std::cout << st;
        ifs.close();
        return 0;
    }
#endif
    if (!script.empty()) {
        if (!std::filesystem::exists(script)) {
            std::cerr << script << ": No such file or directory" << std::endl;
            return -1;
        }
        if (freopen(script.c_str(), "r", stdin) == nullptr) {
            std::cerr << "can not open script file '" << "'." << std::endl;
            return -1;
        }
    }
    char *cmd;
    rl_attempted_completion_function = fileman_completion;
    for (cmd = readline(">>>"); cmd; cmd = readline(">>>")) {
        add_history(cmd);
        exec(cmd);
        free(cmd);
    }
    std::cout << std::endl;
    return 0;
}