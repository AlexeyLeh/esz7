//
// Created by Lehtin A.V. on 09.08.2026
//

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

#define LOG_MAX_SIZE    (10 * 1024 * 1024) // 10 МБ
#define LOG_MAX_FILES   5
#define LOG_MSG_LEN     (1024*2)

typedef enum log_type_t {
    INFO = 0,
    ERROR = 1,
    CRITICAL = 2,
    FATAL = 3,
} log_type_t;

//---------------------------------------------------------
#define LOG(LEVEL, FORMAT, ...)  \
do { \
    if (log_enabled()) { \
        char _buf_[LOG_MSG_LEN]; \
        snprintf(_buf_, sizeof(_buf_), FORMAT, ##__VA_ARGS__); \
        log_lock(); \
        log_message(LEVEL, _buf_); \
        log_unlock(); \
    } \
} while(0)

//---------------------------------------------------------
void log_open_file(const char *logpath);
const char *log_fname();
void log_set_enabled(bool enabled);
bool log_enabled();
void log_lock();
void log_unlock();
void log_message(log_type_t level, const char *msg);
void close_log();

#endif //LOG_H
