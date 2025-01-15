#ifndef PROC_H_
#define PROC_H_

#include <stddef.h>
#include <unistd.h>
#include <spawn.h>

typedef struct proc_h {
    pid_t pid;
    const char* path;
    int argc;
    const char** argv;
    int stdout_pipe[2];
    int stderr_pipe[2];
} proc_h;

/* Init handle to process at _path */
proc_h* proc_init(const char* _path, int _argc, const char** _argv);

/* Spawn process */
bool proc_start(proc_h* _handle);

/* Kill process */
bool proc_stop(proc_h* _handle);

#endif // PROC_H_