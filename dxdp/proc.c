#include "dxdp/proc.h"
#include "dxdp/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <spawn.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

extern char **environ;

typedef struct {
    int fd;
    int type;
} pipe_info_t;

// TODO: COMPILER bug..only stderr works
// TODO: We must encapsulate this anyway instead of having
// proc.c != compiler ...
void handle_child_output(const char* c, int type) {
    if (type == 0){
        log_write(LL_INFO, "[COMPILER] %s", c);
   } else {
        log_write(LL_INFO, "[COMPILER] %s", c);
   }
}

void* read_pipe(void *arg) {
    pipe_info_t *info = (pipe_info_t *)arg;
    int fd = info->fd;
    int type = info->type;
    char buffer[1024];
    ssize_t count;

    while ((count = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';
        handle_child_output(buffer, type);
    }

    if (count == -1) { } //ERROR

    close(fd);
    free(info);
    return NULL;
}

proc_h* proc_init(const char* _path, int _argc, const char** _argv) {
    proc_h* handle = malloc(sizeof(proc_h));
    if(!handle) {
        return NULL;
    }

    handle->pid = -1;
    handle->path = _path;
    handle->argc = _argc;
    handle->argv = _argv;

    if (pipe(handle->stdout_pipe) != 0) {
        free(handle);
        return NULL;
    }

    if (pipe(handle->stderr_pipe) != 0) {
        close(handle->stdout_pipe[0]);
        close(handle->stdout_pipe[1]);
        free(handle);
        return NULL;
    }

    return handle;
}

bool proc_start(proc_h* _handle) {
    if (_handle->pid != -1) {
        log_write(LL_ERROR, "Child proc PID: %d is already running.\n",
            _handle->pid);
        return false;
    }

    char** spawn_argv = NULL;
    if (_handle->argc > 0) {
        spawn_argv = malloc((_handle->argc + 1) * sizeof(char*));
        if (spawn_argv == NULL) {
            return false;
        }

        for (int i = 0; i < _handle->argc; ++i) {
            spawn_argv[i] = (char*)_handle->argv[i];
        }
        spawn_argv[_handle->argc] = NULL;
    } else {
        spawn_argv = malloc(sizeof(char*));
        if (spawn_argv == NULL) {
            return false;
        }
        spawn_argv[0] = NULL;
    }

    posix_spawn_file_actions_t actions;
    int fa_init = posix_spawn_file_actions_init(&actions);
    if (fa_init != 0) {
        free(spawn_argv);
        return false;
    }

    int fa_dup2_out = posix_spawn_file_actions_adddup2(&actions,
        _handle->stdout_pipe[1], STDOUT_FILENO);
    if (fa_dup2_out != 0) {
        posix_spawn_file_actions_destroy(&actions);
        free(spawn_argv);
        return false;
    }

    int fa_dup2_err = posix_spawn_file_actions_adddup2(&actions,
        _handle->stderr_pipe[1], STDERR_FILENO);
    if (fa_dup2_err != 0) {
        posix_spawn_file_actions_destroy(&actions);
        free(spawn_argv);
        return false;
    }

    int fa_close_out = posix_spawn_file_actions_addclose(&actions,
        _handle->stdout_pipe[0]);
    if (fa_close_out != 0) {
        posix_spawn_file_actions_destroy(&actions);
        free(spawn_argv);
        return false;
    }

    int fa_close_err = posix_spawn_file_actions_addclose(&actions,
        _handle->stderr_pipe[0]);
    if (fa_close_err != 0) {
        posix_spawn_file_actions_destroy(&actions);
        free(spawn_argv);
        return false;
    }
    
    /* TODO: Compiler does not need to nor should run as ROOT.
     * 
     * posix_spawn does not allow for setting uid, euid, etc.
     * 
     * Calling setuid in an already root spawned process significantly
     * decreases security compared to not running that process as root at all.
     * 
     * Especially considering clang's massive memory bugs.
     * 
     * Tricky is needed!
     */
    pid_t pid;
    int status = posix_spawn(&pid, _handle->path, &actions, NULL, spawn_argv,
        environ); 
    posix_spawn_file_actions_destroy(&actions);

    free(spawn_argv);

    if (status != 0) {
        close(_handle->stdout_pipe[1]);
        close(_handle->stderr_pipe[1]);
        return false;
    }

    _handle->pid = pid;
    log_write(LL_INFO, "Child proc started with PID %d.\n", pid);

    close(_handle->stdout_pipe[1]);
    close(_handle->stderr_pipe[1]);

    pipe_info_t* stdout_info = malloc(sizeof(pipe_info_t));
    if (stdout_info == NULL) {
        return false;
    }
    stdout_info->fd = _handle->stdout_pipe[0];
    stdout_info->type = 0;

    pipe_info_t* stderr_info = malloc(sizeof(pipe_info_t));
    if (stderr_info == NULL) {
        free(stdout_info);
        return false;
    }
    stderr_info->fd = _handle->stderr_pipe[0];
    stderr_info->type = 1;

    pthread_t stdout_thread, stderr_thread;
    if (pthread_create(&stdout_thread, NULL, read_pipe, stdout_info) != 0) {
        free(stdout_info);
        free(stderr_info);
        return false;
    }
    if (pthread_create(&stderr_thread, NULL, read_pipe, stderr_info) != 0) {
        pthread_cancel(stdout_thread);
        free(stdout_info);
        free(stderr_info);
        return false;
    }

    // Will clean themselves
    pthread_detach(stdout_thread);
    pthread_detach(stderr_thread);

    return true;
}

bool proc_stop(proc_h* _handle) {
    if (kill(_handle->pid, SIGTERM) != 0) {
       log_write(LL_INFO, "Failed to send SIGTERM to child proc");
        return false;
    }
    log_write(LL_INFO, "SIGTERM sent to PID %d.\n", _handle->pid);
    log_write(LL_INFO, "Waiting for death...\n", _handle->pid);

    int status;
    waitpid(_handle->pid, &status, WUNTRACED);

    log_write(LL_INFO, "Killed\n", _handle->pid);
    close(_handle->stdout_pipe[0]);
    close(_handle->stderr_pipe[0]);
    return true;
}