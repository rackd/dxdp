#ifndef LOG_H_
#define LOG_H_

#include <linux/limits.h>
#include <stdio.h>

#define LOG_OUTPUT_FILE 0x1
#define LOG_OUTPUT_STD  0x2

typedef enum ll_level {
    LL_TRACE = 0,
    LL_DEBUG = 1,
    LL_INFO  = 2,
    LL_WARN  = 3,
    LL_ERROR = 4,
    LL_FATAL = 5,
} ll_level;

typedef struct log_h {
    bool path_set;
    char file_path[PATH_MAX];
    int out_flags;
    FILE* file_stream;
    ll_level min_log_level;
} log_h;

/* Init log. */
bool log_init();

/* Set file logger path. */
void log_set_path(const char* _path);

/* Set log flags */
void log_set_flags(int flags);

/* Write to log */
void log_write(ll_level _level, const char* _format, ...);

/* Set min log output level. Anything under _level will not be logged. */
void log_set_output_level(ll_level _level);

void log_close();


#endif // LOG_H_