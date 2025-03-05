#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"
#include "config.h"

void log_message(LogLevel level, const char* format, ...) {
    
    if (!config_get_show_log() && level != ERROR && level != FATAL) {
        return;
    }
    
    
    time_t current_time;
    struct tm* time_info;
    char time_string[20];
    
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
    
    
    const char* level_str;
    switch (level) {
        case DEBUG:   level_str = "DEBUG"; break;
        case INFO:    level_str = "INFO"; break;
        case WARN: level_str = "WARN"; break;
        case ERROR:   level_str = "ERROR"; break;
        case FATAL:level_str = "FATAL"; break;
        default:      level_str = "UNKNOWN"; break;
    }
    
    
    printf("[%s] [%s] ", time_string, level_str);
    
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    
    printf("\n");
    
    
    fflush(stdout);
}