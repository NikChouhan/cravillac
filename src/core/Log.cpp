#include <iostream>
#include <iomanip>
#include "Log.h"

std::vector<Log::LogMessageData> Log::m_messages;
bool Log::m_initialized = false;
    
void Log::Init()
{
    m_initialized = true;
}

void Log::Shutdown()
{
    m_initialized = false;
}

void Log::Info(const std::string &msg)
{
    LogMessage(msg, LogLevel::Info);
}

void Log::Warn(const std::string &msg)
{
    LogMessage(msg, LogLevel::Warn);
}

void Log::Error(const std::string &msg)
{
    LogMessage(msg, LogLevel::Error);
}

void Log::LogMessage(const std::string &msg, LogLevel level)
{
    if (!m_initialized)
    {
        return;
    }

    LogMessageData data;
    data.message = msg;
    data.level = level;

    m_messages.push_back(data);

    switch (level)
    {
    case LogLevel::Info:
        std::cout << "\033[32mINFO: " << msg << "\033[0m" << std::endl;
        break;
    case LogLevel::InfoDebug:
        std::cout << "\033[36mINFO_DEBUG: " << msg << "\033[0m" << std::endl;
        break;
    case LogLevel::Warn:
        std::cout << "\033[33mWARN: " << msg << "\033[0m" << std::endl;
        break;
    case LogLevel::Error:
        std::cout << "\033[31mERROR: " << msg << "\033[0m" << std::endl;
        break;
    default:
        break;
    }
}

std::string Log::FormatLogs(const std::string& msg, const DirectX::XMMATRIX& matrix)
{
    std::ostringstream oss;
    oss << msg << ":\n\n";
    for (int i = 0; i < 4; ++i)
    {
        DirectX::XMFLOAT4 row;
        DirectX::XMStoreFloat4(&row, matrix.r[i]);
        oss << std::fixed << std::setprecision(2)
            << "[" << row.x << ", " << row.y << ", " << row.z << ", " << row.w << "]\n";
    }
    return oss.str();
}

std::string Log::FormatLogs(const std::string& msg, const DirectX::XMVECTOR& vector)
{
    std::ostringstream oss;
    DirectX::XMFLOAT4 vec;
    DirectX::XMStoreFloat4(&vec, vector);
    oss << msg << ": ["
        << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "]";
    return oss.str();
}