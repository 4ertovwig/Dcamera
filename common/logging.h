/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

typedef enum _LogLevel {
    SYMBOL = 0,
    EMPTY,
    FATAL,
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    DEVEL
} LogLevel;

#ifdef __cplusplus
extern "C" {
#endif
    void log_symbol(LogLevel lvl, char fmt);
    void log_msg(LogLevel lvl, const char *fmt, ...);
    void log_set_lvl(LogLevel lvl);
#ifdef __cplusplus
}
#endif

#define LOG_SYMBOL(fmt)          log_symbol(SYMBOL, fmt)
#define LOG_EMPTY(fmt, ...)      log_msg(EMPTY, fmt, ##__VA_ARGS__)
#define LOG_DEV(fmt, ...)        log_msg(DEVEL, "%s:%d:%s() " fmt , __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)      log_msg(DEBUG, "%s:%d:%s() " fmt , __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)       log_msg(INFO, fmt , ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...)    log_msg(WARNING, "%s() " fmt , __FUNCTION__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)      log_msg(ERROR, "%s() " fmt , __FUNCTION__, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)      log_msg(FATAL, "%s() " fmt , __FUNCTION__, ##__VA_ARGS__)

// Absolutely all messages(in developer mode)
#define LOG_SET_DEVEL()      log_set_lvl(DEVEL);
// All extra messages
#define LOG_SET_DEBUG()      log_set_lvl(DEBUG);
// Only critical messages
#define LOG_SET_CRITICAL()   log_set_lvl(FATAL);
