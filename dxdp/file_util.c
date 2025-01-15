#include "dxdp/file_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** file_read(const char* _path, size_t _buffer_size, size_t* _num_lines) {
    FILE* file = fopen(_path, "r");
    if (!file) {
        return NULL;
    }

    char buffer[_buffer_size];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
    if (bytesRead == _buffer_size && !feof(file)) {
        fprintf(stderr, "File was too large for buffer\n");
        return NULL;
    }

    fclose(file);
    buffer[bytesRead] = '\0';

    size_t num_lines = 0;
    for (size_t i = 0; i < bytesRead; ++i) {
        if (buffer[i] == '\n') {
            num_lines++;
        }
    }

    char** lines = malloc(sizeof(char*) * (num_lines + 1));
    if (!lines) {
        return NULL;
    }

    size_t c_line = 0;
    char* start = buffer;
    for (size_t i = 0; i < bytesRead; ++i) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            lines[c_line++] = strdup(start);
            start = buffer + i + 1;
        }
    }

    if (start < buffer + bytesRead) {
        lines[c_line++] = strdup(start);
    }

    *_num_lines = c_line;
    return lines;
}

void file_read_free(char** _lines, size_t _num_lines) {
    for (size_t i = 0; i < _num_lines; i++) {
        free(_lines[i]);
    }
    free(_lines);
}