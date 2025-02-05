#ifndef LOG_H
#define LOG_H

#include <string>
#include <vector>
#include <sstream>
#include <DirectXMath.h>

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

    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);

    template <typename T>
    static void InfoDebug(const std::string& msg, const T& value)
    {
        LogMessage(FormatLogs(msg, value), LogLevel::InfoDebug);
    }

private:

    struct LogMessageData
    {
        std::string message;
        LogLevel level;
    };
    static void LogMessage(const std::string& msg, LogLevel level);

    template <typename T>
    static std::string FormatLogs(const std::string& msg, const T& value)
    {
        std::ostringstream oss;
        oss << msg << ": " << value;
        return oss.str();
    }

    static std::string FormatLogs(const std::string& msg, const DirectX::XMMATRIX& matrix);
    static std::string FormatLogs(const std::string& msg, const DirectX::XMVECTOR& vector);

    static std::vector<LogMessageData> m_messages;
    static bool m_initialized;
};

#endif // LOG_H