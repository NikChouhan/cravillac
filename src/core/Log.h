#ifndef LOG_H
#define LOG_H

#include <string>

#include <vector>
#include <sstream>
#include "StandardTypes.h"

#include <format>

class Log
{
public:
    enum class LogLevel
    {
        Info,
        InfoDebug,
        Warn,
        Error
    };

    static void Init();
    static void Shutdown();
    template<typename... Args>
    static void PrintL(LogLevel level, std::format_string<Args...> format, Args&&... args)
    {
        std::string message = std::format(format, std::forward<Args>(args)...);
        LogMessage(level, message.c_str());
    }
private:
    struct LogMessageData
    {
        std::string message;
        LogLevel level;
    };
    static void LogMessage(LogLevel level, cstring msg);
    static bool m_initialized;
}; 
#ifdef _WIN32
#define printl(level, format, ...) Log::PrintL(level, format, __VA_ARGS__)
#endif
#ifdef USE_WAYLAND
#define printl(level, format, ...) Log::PrintL(level, format __VA_OPT__(,) __VA_ARGS__)
#endif

#endif// LOG_H