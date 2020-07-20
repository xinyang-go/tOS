//
// Created by xinyang on 2020/6/22.
//

#include "register.h"

#ifdef WITH_BOOST_STACKTRACE

#include <csignal>
#include <boost/stacktrace.hpp>

void signal_stacktrace(int signum) {
    signal(signum, SIG_DFL);
    boost::stacktrace::safe_dump_to("stacktrace.dump");
    std::cout << "stacktrace:" << std::endl;
    std::cout << boost::stacktrace::stacktrace();
    exit(signum);
}

void register_signal_stacktrace() {
    signal(SIGSEGV, signal_stacktrace);
    signal(SIGABRT, signal_stacktrace);
    signal(SIGILL, signal_stacktrace);
    signal(SIGFPE, signal_stacktrace);
}

EXEC_EXPORT(register_signal_stacktrace);

#endif
