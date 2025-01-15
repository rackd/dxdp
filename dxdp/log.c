#include "dxdp/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>

static log_h* global_logger = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void _log_get_ts(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t_info);
}

static const char* _log_ll_2_str(ll_level level) {
    switch(level) {
        case LL_TRACE: return "TRACE";
        case LL_DEBUG: return "DEBUG";
        case LL_INFO:  return "INFO";
        case LL_WARN:  return "WARN";
        case LL_ERROR: return "ERROR";
        case LL_FATAL: return "FATAL";
        default:       return "UNKNOWN";
    }
}

static bool _log_open_file(log_h* _handle) {
    if (!(_handle->out_flags & LOG_OUTPUT_FILE)) {
        return false;
    }
    if (_handle->file_stream != NULL) {
        return true;
    }
    _handle->file_stream = fopen(_handle->file_path, "a");
    if (!_handle->file_stream) {
        fprintf(stderr, "Failed to open log file: %s\n", _handle->file_path);
        return false;
    }
    return true;
}

static void _log_close_file(log_h* _handle) {
    if (_handle->file_stream) {
        fclose(_handle->file_stream);
        _handle->file_stream = NULL;
    }
}

bool log_init() {
    pthread_mutex_lock(&log_mutex);

    if (global_logger != NULL) {
        pthread_mutex_unlock(&log_mutex);
        return true;
    }

    global_logger = malloc(sizeof(log_h));
    if (!global_logger) {
        fprintf(stderr, "Failed to allocate memory for logger\n");
        pthread_mutex_unlock(&log_mutex);
        return false;
    }

    global_logger->path_set = false;
    global_logger->file_path[0] = '\0';
    global_logger->out_flags = LOG_OUTPUT_STD;
    global_logger->file_stream = NULL;
    global_logger->min_log_level = LL_TRACE;

    pthread_mutex_unlock(&log_mutex);
    return true;
}

void log_set_path(const char* _path) {
    pthread_mutex_lock(&log_mutex);

    if (global_logger == NULL) {
        fprintf(stderr, "Logger not initialized. Call log_init() first.\n");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    strncpy(global_logger->file_path, _path, PATH_MAX - 1);
    global_logger->file_path[PATH_MAX - 1] = '\0';
    global_logger->path_set = true;

    if (global_logger->out_flags & LOG_OUTPUT_FILE) {
        if (!_log_open_file(global_logger)) {
            global_logger->out_flags &= ~LOG_OUTPUT_FILE;
        }
    }

    pthread_mutex_unlock(&log_mutex);
}

void log_set_flags(int flags) {
    pthread_mutex_lock(&log_mutex);

    if (global_logger == NULL) {
        fprintf(stderr, "Logger not initialized. Call log_init() first.\n");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    bool was_file_output = global_logger->out_flags & LOG_OUTPUT_FILE;
    bool wants_file_output = flags & LOG_OUTPUT_FILE;

    global_logger->out_flags = flags;

    if (wants_file_output && global_logger->path_set) {
        if (!was_file_output) { // Only attempt if it wasn't open before
            if (!_log_open_file(global_logger)) {
                global_logger->out_flags &= ~LOG_OUTPUT_FILE;
            }
        }
    } else if (!wants_file_output && was_file_output) {
        _log_close_file(global_logger);
    }

    pthread_mutex_unlock(&log_mutex);
}

void log_set_output_level(ll_level _level) {
    pthread_mutex_lock(&log_mutex);

    if (global_logger == NULL) {
        fprintf(stderr, "Logger not initialized. Call log_init() first.\n");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    if (_level < LL_TRACE || _level > LL_FATAL) {
        fprintf(stderr,"Invalid log level: %d. Must be between LL_TRACE (0)"
            " and LL_FATAL (5).\n", _level);
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    global_logger->min_log_level = _level;

    pthread_mutex_unlock(&log_mutex);
}

void log_write(ll_level _level, const char* _format, ...) {
    pthread_mutex_lock(&log_mutex);

    if (global_logger == NULL) {
        fprintf(stderr, "Logger not initialized. Call log_init() first.\n");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    if (_level < global_logger->min_log_level) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    char timestamp[20];
    _log_get_ts(timestamp, sizeof(timestamp));

    const char* level_str = _log_ll_2_str(_level);
    char message[1024];
    va_list args;
    va_start(args, _format);
    vsnprintf(message, sizeof(message), _format, args);
    va_end(args);
    char log_entry[1200];
    snprintf(log_entry, sizeof(log_entry), "[%s] [%s] %s", timestamp,
        level_str, message);

    if (global_logger->out_flags & LOG_OUTPUT_STD) {
        FILE* out_stream = stdout;
        if (_level >= LL_ERROR) {
            out_stream = stderr;
        }
        fputs(log_entry, out_stream);
        fflush(out_stream);
    }

    if ((global_logger->out_flags & LOG_OUTPUT_FILE)
    && global_logger->path_set) {
        if (_log_open_file(global_logger)) {
            fputs(log_entry, global_logger->file_stream);
            fflush(global_logger->file_stream);
        }
    }

    pthread_mutex_unlock(&log_mutex);
}

void log_close() {
    pthread_mutex_lock(&log_mutex);

    if (global_logger == NULL) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    _log_close_file(global_logger);
    free(global_logger);
    global_logger = NULL;

    pthread_mutex_unlock(&log_mutex);
}