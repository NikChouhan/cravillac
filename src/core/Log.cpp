#include <iostream>
#include <iomanip>
#include "Log.h"

#include <cstdarg>
#include <cstdio>
#include "common.h"
#include <print>

namespace CV
{
    bool Log::m_initialized = false;
    static constexpr u32 bufferSize = 1024 * 1024;
    static char logBuffer[bufferSize];

    void Log::Init()
    {
        m_initialized = true;
    }

    void Log::Shutdown()
    {
        m_initialized = false;
    }

    void Log::LogMessage(LogLevel level, cstring msg)
    {
        if (!m_initialized)
        {
            return;
        }

        switch (level)
        {
        default: break;
        case LogLevel::Info:
            std::println("\033[32m{}\033[0m", msg);
            break;
        case LogLevel::Warn:
            std::println("\033[33m{}\033[0m", msg);
            break;
        case LogLevel::Error:
            std::println(("\033[31m{}\033[0m"), msg);
            break;
        case LogLevel::InfoDebug:
            std::println(("\033[36m{}\033[0m"), msg);
            break;
        }
    }
}