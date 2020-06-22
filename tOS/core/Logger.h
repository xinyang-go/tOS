//
// Created by xinyang on 2020/4/4.
//

#ifndef _TOS_LOGGER_HPP_
#define _TOS_LOGGER_HPP_

#include "../tOS_config.h"
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <ctime>

/************** Define the control code *************/
#define START_CTR           "\033[0"
#define END_CTR             "m"
#define CLEAR_CODE          ";0"
#define LIGHT_CODE          ";1"
#define LINE_CODE           ";4"
#define BLINK_CODE          ";5"
#define REVERSE_CODE        ";7"
#define VANISH_CODE         ";8"
#define WORD_WHITE_CODE     ";30"
#define WORD_RED_CODE       ";31"
#define WORD_GREEN_CODE     ";32"
#define WORD_YELLOW_CODE    ";33"
#define WORD_BLUE_CODE      ";34"
#define WORD_PURPLE_CODE    ";35"
#define WORD_CYAN_CODE      ";36"
#define WORD_GRAY_CODE      ";37"
#define BACK_WHITE_CODE     ";40"
#define BACK_RED_CODE       ";41"
#define BACK_GREEN_CODE     ";42"
#define BACK_YELLOW_CODE    ";43"
#define BACK_BLUE_CODE      ";44"
#define BACK_PURPLE_CODE    ";45"
#define BACK_CYAN_CODE      ";46"
#define BACK_GRAY_CODE      ";47"

#define CTRS(ctrs)          START_CTR ctrs END_CTR

#define WORD_WHITE          CTRS(WORD_WHITE_CODE)
#define WORD_RED            CTRS(WORD_RED_CODE)
#define WORD_GREEN          CTRS(WORD_GREEN_CODE)
#define WORD_YELLOW         CTRS(WORD_YELLOW_CODE)
#define WORD_BLUE           CTRS(WORD_BLUE_CODE)
#define WORD_PURPLE         CTRS(WORD_PURPLE_CODE)
#define WORD_CYAN           CTRS(WORD_CYAN_CODE)
#define WORD_GRAY           CTRS(WORD_GRAY_CODE)
#define WORD_LIGHT_WHITE    CTRS(LIGHT_CODE WORD_WHITE)
#define WORD_LIGHT_RED      CTRS(LIGHT_CODE WORD_RED)
#define WORD_LIGHT_GREEN    CTRS(LIGHT_CODE WORD_GREEN)
#define WORD_LIGHT_YELLOW   CTRS(LIGHT_CODE WORD_YELLOW)
#define WORD_LIGHT_BLUE     CTRS(LIGHT_CODE WORD_BLUE)
#define WORD_LIGHT_PURPLE   CTRS(LIGHT_CODE WORD_PURPLE)
#define WORD_LIGHT_CYAN     CTRS(LIGHT_CODE WORD_CYAN)
#define WORD_LIGHT_GRAY     CTRS(LIGHT_CODE WORD_GRAY)
#define CLEAR_ALL           CTRS(CLEAR_CODE)

#define STR_CTR(ctrs, str)  ctrs str CLEAR_ALL

namespace tOS {

    enum class LogLevel {
        NONE = 0, ERROR, WARNING, INFO
    };

    using ColorMap_t = std::unordered_map<LogLevel, std::string>;

    inline LogLevel global_log_level{LogLevel::INFO};
    inline const ColorMap_t global_log_color{{LogLevel::INFO,        WORD_GRAY},
                                             {LogLevel::WARNING,     WORD_YELLOW},
                                             {LogLevel::ERROR,   WORD_RED}};
    inline std::mutex global_log_mtx;

    /* 用于打印log的对象
     * LOG_LEVEL: 当前输出等级
     * LOCK: 是否加锁打印
     */
    template<LogLevel LOG_LEVEL, bool LOCK = TOS_LOG_LOCK_DEFAULT>
    class LoggerPrinter : public std::ostream {
        friend class Logger;

    private:
        static constexpr const char *level_tag() {
            if constexpr (LOG_LEVEL == LogLevel::INFO) {
                return "INFO";
            } else if constexpr (LOG_LEVEL == LogLevel::WARNING) {
                return "WARNING";
            } else if constexpr (LOG_LEVEL == LogLevel::ERROR) {
                return "ERROR";
            } else {
                return "undefined";
            }
        }

        const LogLevel &local_log_level;

        // 私有构造使得该类不能被直接创建。
        LoggerPrinter(const LogLevel &lv, const std::string &name) : std::ostream(), local_log_level(lv) {
            if (LOG_LEVEL <= local_log_level && LOG_LEVEL <= global_log_level)
                this->rdbuf(std::cout.rdbuf());
            if constexpr(LOCK) global_log_mtx.lock();
            *this << global_log_color.find(LOG_LEVEL)->second << '[' << name << "] <" << level_tag() << ">: ";
        }

        LoggerPrinter(const LoggerPrinter &) = delete;

        LoggerPrinter &operator=(const LoggerPrinter &) = delete;

    public:
        ~LoggerPrinter() override {
            *this << CLEAR_ALL;
            if constexpr(LOCK) global_log_mtx.unlock();
        }
    };

    class Logger {
    public:
        LogLevel local_log_level;
    private:
        const std::string name;
        std::unordered_map<std::string, int> fps_cnt;
        std::unordered_map<std::string, int> fps_time;

        template<LogLevel LOG_LEVEL>
        LoggerPrinter<LOG_LEVEL> log() {
            return LoggerPrinter<LOG_LEVEL>(local_log_level, name);
        }

    public:
        explicit Logger(std::string n, LogLevel l = LogLevel::INFO) : local_log_level(l), name(std::move(n)) {};

        int cnt_fps(const std::string &tag) {
            int fps = -1;
            fps_cnt[tag]++;
            if (time(nullptr) != fps_time[tag]) {
                fps_time[tag] = time(nullptr);
                log_i() << "fps-" << tag << ": " << fps_cnt[tag] << std::endl;
                fps = fps_cnt[tag];
                fps_cnt[tag] = 0;
            }
            return fps;
        }

        LoggerPrinter<LogLevel::INFO> log_i() {
            return log<LogLevel::INFO>();
        }

        LoggerPrinter<LogLevel::WARNING> log_w() {
            return log<LogLevel::WARNING>();
        }

        LoggerPrinter<LogLevel::ERROR> log_e() {
            return log<LogLevel::ERROR>();
        }
    };
}


#endif /* _TOS_LOGGER_HPP_ */
