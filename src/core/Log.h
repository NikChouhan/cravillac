#ifndef LOG_H
#define LOG_H

#include <string>

#include <vector>
#include <sstream>
#include "StandardTypes.h"

namespace Cravillac
{
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
}

#define printl(level, format, ...) Cravillac::Log::PrintL(level, format, __VA_ARGS__);

#endif// LOG_H