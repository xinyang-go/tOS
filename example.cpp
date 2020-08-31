#include <iostream>
#include <thread>
#include <chrono>
#include "tOS.h"

using namespace std::chrono;
using namespace tOS;

void print_log(const char *tag) {
    auto logger = Node::this_node()->make_logger();
    logger->log_i() << tag << std::endl;
    logger->log_w() << tag << std::endl;
    logger->log_e() << tag << std::endl;
}

using c_time_t = decltype(std::chrono::high_resolution_clock::now());

int publisher(int argc, const char *argv[]) {
    const auto node = Node::this_node();
    print_log("publisher");
    auto logger = node->make_logger();
    auto p = node->make_publisher<OpenMode::FIND_OR_CREATE, c_time_t, 1>("timeval");
    std::this_thread::sleep_for(1000ms);
    c_time_t t1;
    while (node->running) {
        t1 = std::chrono::high_resolution_clock::now();
        p.push(t1);
        std::this_thread::sleep_for(1s);
    }
    return 0;
}

ENTRY_EXPORT(publisher);

int subscriber(int argc, const char *argv[]) {
    auto node = Node::this_node();
    print_log("subscriber");
    auto logger = node->make_logger();
    auto s = node->make_subscriber<OpenMode::FIND_OR_CREATE, c_time_t, 1>("timeval");
    c_time_t t1{}, t2{};
    while (node->running) {
        if (s.pop(t1, 2s) != MessageStatus::OK) continue;
        t2 = std::chrono::high_resolution_clock::now();
        logger->log_i() << "dt: " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
                        << "us" << std::endl;
    }
    return 0;
}

ENTRY_EXPORT(subscriber);

int server(int argc, const char *argv[]) {
    auto node = Node::this_node();
    print_log("server");
    auto logger = node->make_logger();
    auto s = node->make_server<OpenMode::FIND_OR_CREATE, c_time_t, c_time_t, 1>("timeval");
    c_time_t tm;
    decltype(s)::T t;
    while (node->running) {
        if (!s.pop(t, 2s)) continue;
        auto &[ts, p] = t;
        tm = std::chrono::high_resolution_clock::now();
        logger->log_i() << "ds: " << std::chrono::duration_cast<std::chrono::microseconds>(tm - ts).count()
                        << "us" << std::endl;
        p.set_value(tm);
    }
    return 0;
}

ENTRY_EXPORT(server);

int client(int argc, const char *argv[]) {
    auto node = Node::this_node();
    print_log("client");
    auto logger = node->make_logger();
    auto c = node->make_client<OpenMode::FIND_OR_CREATE, c_time_t, c_time_t, 1>("timeval");
    c_time_t ts{}, tm{}, te{};
    while (node->running) {
        ts = std::chrono::high_resolution_clock::now();
        auto f = c.emplace(ts);
        try {
            tm = f.get();
        } catch (std::future_error &e) {
            continue;
        }
        te = std::chrono::high_resolution_clock::now();
        logger->log_i() << "dr: " << std::chrono::duration_cast<std::chrono::microseconds>(te - ts).count()
                        << "us" << std::endl;
        std::this_thread::sleep_for(800ms);
    }
    return 0;
}

ENTRY_EXPORT(client);

int sync_waiter(int argc, const char *argv[]) {
    auto node = Node::this_node();
    auto logger = node->make_logger();

    auto sync = node->make_sync<OpenMode::FIND_OR_CREATE, char>("sync");
    if (!sync) {
        logger->log_e() << "sync create fail!" << std::endl;
    }

    while (node->running) {
        sync->wait('e');
        logger->log_i() << "current mode: 'e'" << std::endl;
        std::this_thread::sleep_for(0.5s);
    }

    return 0;
}

ENTRY_EXPORT(sync_waiter);

int sync_setter(int argc, const char *argv[]) {
    auto node = Node::this_node();
    auto logger = node->make_logger();

    auto sync = node->make_sync<OpenMode::FIND_OR_CREATE, char>("sync");
    if (!sync) {
        logger->log_e() << "sync create fail!" << std::endl;
    }

    int cnt = 0;
    while (node->running) {
        if (++cnt % 3 == 0) {
            logger->log_i() << "set mode to 'e'." << std::endl;
            sync->update('e');
        } else {
            logger->log_i() << "set mode to 'a'." << std::endl;
            sync->update('a');
        }
        std::this_thread::sleep_for(1s);
    }

    return 0;
}

ENTRY_EXPORT(sync_setter);

int my_stacktrace_test(int argc, const char *argv[]) {
    throw std::runtime_error("test stacktrace.");
    return 0;
}

ENTRY_EXPORT_ALIAS(my_stacktrace_test, stacktrace);
