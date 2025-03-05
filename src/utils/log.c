#include "log.h"

static int debug_enabled = 1;

void set_log_debug_enabled(int enabled) {
    debug_enabled = enabled;
}

void log_message(LogLevel level, const char *format, ...) {
    const char *level_str;
    switch (level) {
        case LOG_LEVEL_INFO:  level_str = "INFO"; break;
        case LOG_LEVEL_WARN:  level_str = "WARN"; break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case LOG_LEVEL_FATAL: level_str = "FATAL"; break;
        case LOG_LEVEL_DEBUG: 
            if (!debug_enabled) return;
            level_str = "DEBUG"; 
            break;
        default: level_str = "LOG";
    }

    va_list args;
    va_start(args, format);
    printf("[%s] ", level_str);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
