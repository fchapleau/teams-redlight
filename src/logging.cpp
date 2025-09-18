#include "logging.h"
#include <cstdarg>

// Static member initialization
int Logger::currentLevel = LOG_LEVEL;

void Logger::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    while (!Serial && millis() < 5000) {
        delay(100);
    }
    infof("=== Teams Red Light Logger Initialized (Level: %d) ===", currentLevel);
}

void Logger::setLevel(int level) {
    currentLevel = level;
    infof("Log level changed to: %d", level);
}

int Logger::getLevel() {
    return currentLevel;
}

void Logger::debug(const String& message) {
    logMessage(LOG_LEVEL_DEBUG, "", message);
}

void Logger::info(const String& message) {
    logMessage(LOG_LEVEL_INFO, "", message);
}

void Logger::warn(const String& message) {
    logMessage(LOG_LEVEL_WARN, "", message);
}

void Logger::error(const String& message) {
    logMessage(LOG_LEVEL_ERROR, "", message);
}

void Logger::debug(const String& component, const String& message) {
    logMessage(LOG_LEVEL_DEBUG, component, message);
}

void Logger::info(const String& component, const String& message) {
    logMessage(LOG_LEVEL_INFO, component, message);
}

void Logger::warn(const String& component, const String& message) {
    logMessage(LOG_LEVEL_WARN, component, message);
}

void Logger::error(const String& component, const String& message) {
    logMessage(LOG_LEVEL_ERROR, component, message);
}

void Logger::debugf(const char* format, ...) {
    if (currentLevel > LOG_LEVEL_DEBUG) return;
    va_list args;
    va_start(args, format);
    logMessagef(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void Logger::infof(const char* format, ...) {
    if (currentLevel > LOG_LEVEL_INFO) return;
    va_list args;
    va_start(args, format);
    logMessagef(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void Logger::warnf(const char* format, ...) {
    if (currentLevel > LOG_LEVEL_WARN) return;
    va_list args;
    va_start(args, format);
    logMessagef(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void Logger::errorf(const char* format, ...) {
    if (currentLevel > LOG_LEVEL_ERROR) return;
    va_list args;
    va_start(args, format);
    logMessagef(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

void Logger::printTimestamp() {
    unsigned long uptime = millis();
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    Serial.printf("[%02lu:%02lu:%02lu.%03lu] ", hours, minutes, seconds, uptime % 1000);
}

void Logger::printLevel(int level, const char* prefix, const char* color) {
    Serial.print(color);
    Serial.print(prefix);
    Serial.print(LOG_COLOR_RESET);
    Serial.print(" ");
}

void Logger::logMessage(int level, const String& component, const String& message) {
    if (currentLevel > level) return;
    
    printTimestamp();
    
    switch (level) {
        case LOG_LEVEL_DEBUG:
            printLevel(level, LOG_PREFIX_DEBUG, LOG_COLOR_DEBUG);
            break;
        case LOG_LEVEL_INFO:
            printLevel(level, LOG_PREFIX_INFO, LOG_COLOR_INFO);
            break;
        case LOG_LEVEL_WARN:
            printLevel(level, LOG_PREFIX_WARN, LOG_COLOR_WARN);
            break;
        case LOG_LEVEL_ERROR:
            printLevel(level, LOG_PREFIX_ERROR, LOG_COLOR_ERROR);
            break;
    }
    
    if (component.length() > 0) {
        Serial.print("[");
        Serial.print(component);
        Serial.print("] ");
    }
    
    Serial.println(message);
}

void Logger::logMessagef(int level, const char* format, va_list args) {
    printTimestamp();
    
    switch (level) {
        case LOG_LEVEL_DEBUG:
            printLevel(level, LOG_PREFIX_DEBUG, LOG_COLOR_DEBUG);
            break;
        case LOG_LEVEL_INFO:
            printLevel(level, LOG_PREFIX_INFO, LOG_COLOR_INFO);
            break;
        case LOG_LEVEL_WARN:
            printLevel(level, LOG_PREFIX_WARN, LOG_COLOR_WARN);
            break;
        case LOG_LEVEL_ERROR:
            printLevel(level, LOG_PREFIX_ERROR, LOG_COLOR_ERROR);
            break;
    }
    
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.println(buffer);
}