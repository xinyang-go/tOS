cmake_minimum_required(VERSION 3.16)
project(tOS)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Readline REQUIRED)
find_package(Threads REQUIRED)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

# 配置boost::stacktrace
option(WITH_BOOST_STACKTRACE "try to use boost::stacktrace for error analysis." ON)
if (WITH_BOOST_STACKTRACE)
    find_package(Backtrace)
    if (Backtrace_FOUND)
        find_package(Boost COMPONENTS stacktrace_backtrace)
        if (Boost_stacktrace_backtrace_FOUND)
            link_libraries(Boost::stacktrace_backtrace)
            link_libraries(dl ${Backtrace_LIBRARY})
            add_compile_definitions(WITH_BOOST_STACKTRACE)
            add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
        endif ()
    else ()
        find_package(Boost COMPONENTS stacktrace_basic)
        if (Boost_stacktrace_basic_FOUND)
            link_libraries(Boost::stacktrace_basic)
            link_libraries(dl)
            add_compile_definitions(WITH_BOOST_STACKTRACE)
        endif ()
    endif ()
endif ()

# 添加tOS包含路径
include_directories(tOS)

# 添加3rd_party包含路径
include_directories(3rd_party/CLI11/include)
include_directories(3rd_party/pipes/include)
include_directories(3rd_party/fmt/include)
include_directories(3rd_party/tabulate/include)

# 创建fmt连接库
file(GLOB_RECURSE fmt_src 3rd_party/fmt/src/*.cc)
add_library(fmt SHARED ${fmt_src})
link_libraries(fmt)

# 编译主程序
file(GLOB_RECURSE tOS_src tOS/*.cpp)
add_executable(tOS ${tOS_src})
target_include_directories(tOS PRIVATE ${Readline_INCLUDE_DIR})
target_link_libraries(tOS ${Readline_LIBRARY})

# option选择是否编译example
option(BUILD_TOS_EXAMPLE "build the tos example" OFF)
if (BUILD_TOS_EXAMPLE)
    add_library(example OBJECT example.cpp)
    target_link_libraries(tOS example)
endif ()
