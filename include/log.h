#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

typedef enum {
    INFO,
    WARN,
    ERROR,
    FATAL,
    DEBUG
} LogLevel;

void log_message(LogLevel level, const char *format, ...);
void set_log_debug_enabled(int enabled);

#endif
