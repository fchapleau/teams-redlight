#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

// Log levels
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4

// Default log level (can be overridden by build flags)
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Log level colors for better readability (ANSI colors)
#define LOG_COLOR_DEBUG   "\033[36m"  // Cyan
#define LOG_COLOR_INFO    "\033[32m"  // Green
#define LOG_COLOR_WARN    "\033[33m"  // Yellow
#define LOG_COLOR_ERROR   "\033[31m"  // Red
#define LOG_COLOR_RESET   "\033[0m"   // Reset

// Log prefixes
#define LOG_PREFIX_DEBUG  "[DEBUG]"
#define LOG_PREFIX_INFO   "[INFO] "
#define LOG_PREFIX_WARN   "[WARN] "
#define LOG_PREFIX_ERROR  "[ERROR]"

class Logger {
public:
    static void begin(unsigned long baudRate = 115200);
    static void setLevel(int level);
    static int getLevel();
    
    static void debug(const String& message);
    static void info(const String& message);
    static void warn(const String& message);
    static void error(const String& message);
    
    // Formatted logging with parameters
    static void debugf(const char* format, ...);
    static void infof(const char* format, ...);
    static void warnf(const char* format, ...);
    static void errorf(const char* format, ...);
    
    // Log with component/module name
    static void debug(const String& component, const String& message);
    static void info(const String& component, const String& message);
    static void warn(const String& component, const String& message);
    static void error(const String& component, const String& message);

private:
    static int currentLevel;
    static void printTimestamp();
    static void printLevel(int level, const char* prefix, const char* color);
    static void logMessage(int level, const String& component, const String& message);
    static void logMessagef(int level, const char* format, va_list args);
};

// Convenience macros for logging with automatic component detection
#define LOG_DEBUG(msg) Logger::debug(__FUNCTION__, msg)
#define LOG_INFO(msg) Logger::info(__FUNCTION__, msg)
#define LOG_WARN(msg) Logger::warn(__FUNCTION__, msg)
#define LOG_ERROR(msg) Logger::error(__FUNCTION__, msg)

// Formatted logging macros
#define LOG_DEBUGF(fmt, ...) Logger::debugf("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) Logger::infof("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...) Logger::warnf("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) Logger::errorf("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)

#endif // LOGGING_H