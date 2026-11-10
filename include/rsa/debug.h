#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_NONE
} LogLevel;

/* Global configuration */
#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL LOG_LEVEL_DEBUG
#endif

#ifndef LOG_ENABLE
#define LOG_ENABLE 1
#endif

#define LOG_LEVEL_NAME(level) \
    ((level) == LOG_LEVEL_TRACE ? "TRACE" : \
     (level) == LOG_LEVEL_DEBUG ? "DEBUG" : \
     (level) == LOG_LEVEL_INFO  ? "INFO"  : \
     (level) == LOG_LEVEL_WARN  ? "WARN"  : \
     (level) == LOG_LEVEL_ERROR ? "ERROR" : \
     (level) == LOG_LEVEL_FATAL ? "FATAL" : "UNKNOWN")

#define LOG(module, level, fmt, ...) \
    do { \
        if (LOG_ENABLE && (level) >= LOG_MIN_LEVEL) { \
            fprintf(stderr, "[%s] [%s] [%s:%d] " fmt "\n", \
                LOG_LEVEL_NAME(level), \
                module, \
                __FILE__, \
                __LINE__, \
                ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_BIGINT(module, level, msg, num) \
    do { \
        if (LOG_ENABLE && (level) >= LOG_MIN_LEVEL) { \
            char _buffer[1024]; \
            bigint_to_string(num, _buffer, sizeof(_buffer)); \
            LOG(module, level, "%s: %s", msg, _buffer); \
        } \
    } while (0)

#endif
