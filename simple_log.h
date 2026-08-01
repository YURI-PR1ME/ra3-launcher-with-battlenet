/*
 * simple_log.h - Minimal thread-safe logger
 *
 * Levels: 0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=fatal
 * Replaces boost::log
 * Only dependency: C++11 STL + Windows API
 *
 * Usage:
 *   Logger::init("RA3", true, true, 2, 1);
 *   LOG_INFO << "Hello " << 42;
 *   LOG_ERR << "Failed: " << GetLastError();
 */

#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <ctime>
#include <windows.h>

class Logger {
public:
    // Level names with L_ prefix to avoid Windows.h macro conflicts (ERROR, DEBUG)
    enum Level { L_TRACE = 0, L_DEBUG = 1, L_INFO = 2, L_WARN = 3, L_ERROR = 4, L_FATAL = 5 };

    static Logger& instance() {
        static Logger l;
        return l;
    }

    static void init(const std::string& prefix, bool console, bool fileLog,
                     int consoleLevel = L_INFO, int fileLevel = L_DEBUG) {
        Logger& l = instance();
        std::lock_guard<std::mutex> lock(l.mutex_);
        l.prefix_ = prefix;
        l.consoleLevel_ = consoleLevel;
        l.fileLevel_ = fileLevel;

        if (console && !l.console_) {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            HANDLE hOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
            SetStdHandle(STD_OUTPUT_HANDLE, hOut);
            l.console_ = true;
        }

        if (fileLog && !l.file_.is_open()) {
            char ts[64];
            time_t now = time(nullptr);
            struct tm tm_info;
            localtime_s(&tm_info, &now);
            strftime(ts, sizeof(ts), "%Y-%m-%d_%H-%M-%S", &tm_info);

            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s_%s.log", prefix.c_str(), ts);
            l.file_.open(path, std::ios::app);
            l.file_.flush();
        }
    }

    class Line {
    public:
        Line(Level lv, const char* file, int line, const char* func)
            : level_(lv), file_(file), line_(line), func_(func) {}
        ~Line() {
            Logger::instance().write(level_, file_, line_, func_, ss_.str());
        }
        std::ostringstream& stream() { return ss_; }
        template<typename T>
        std::ostringstream& operator<<(const T& val) { ss_ << val; return ss_; }
    private:
        Level level_;
        const char* file_;
        int line_;
        const char* func_;
        std::ostringstream ss_;
    };

    void write(Level lv, const char* file, int line, const char* func, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        char ts[32];
        time_t now = time(nullptr);
        struct tm tm_info;
        localtime_s(&tm_info, &now);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_info);
        const char* lvStr[] = { "TRACE","DEBUG","INFO","WARN","ERROR","FATAL" };
        const char* ls = (lv >= 0 && lv <= 5) ? lvStr[lv] : "?";
        char formatted[2048];
        snprintf(formatted, sizeof(formatted), "[%s %s] %s: %s\n", ts, func, ls, msg.c_str());
        if (console_ && lv >= consoleLevel_) {
            fputs(formatted, stdout);
            fflush(stdout);
        }
        if (file_.is_open() && lv >= fileLevel_) {
            file_ << formatted;
            file_.flush();
        }
    }

private:
    std::mutex mutex_;
    std::string prefix_;
    std::ofstream file_;
    bool console_ = false;
    int consoleLevel_ = L_INFO;
    int fileLevel_ = L_DEBUG;
};

// Log macros using L_ prefixed levels
#define LOG_TRACE Logger::Line(Logger::L_TRACE, __FILE__, __LINE__, __FUNCTION__).stream()
#define LOG_DEBUG Logger::Line(Logger::L_DEBUG, __FILE__, __LINE__, __FUNCTION__).stream()
#define LOG_INFO  Logger::Line(Logger::L_INFO,  __FILE__, __LINE__, __FUNCTION__).stream()
#define LOG_WARN  Logger::Line(Logger::L_WARN,  __FILE__, __LINE__, __FUNCTION__).stream()
#define LOG_ERR   Logger::Line(Logger::L_ERROR, __FILE__, __LINE__, __FUNCTION__).stream()
#define LOG_FATAL Logger::Line(Logger::L_FATAL, __FILE__, __LINE__, __FUNCTION__).stream()
