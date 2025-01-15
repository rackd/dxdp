#ifndef COMPILER_H_
#define COMPILER_H_

#include <stddef.h>
#include <stdbool.h>
#include "dxdp-ipc/ipc.h"

#define MAX_SOURCE_BUFFER_SIZE MAX_ARG_LENGTH * MAX_ARG_COUNT
#define MAX_COMPILED_BUFFER_SIZE MAX_ARG_LENGTH * MAX_ARG_COUNT

bool compile_source(const char* source_code, void* _out_buf,
    size_t* _bin_size);

#endif // COMPILER_H_