#include "dxdp/sysutil.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>

int fd = 0;

bool sys_is_already_running() {
    fd = open(DEFAULT_PID_PATH, O_CREAT | O_RDWR, 0666);

    if(flock(fd, LOCK_EX | LOCK_NB)) {
        return true;
    } else {
        return false;
    }
}

void sys_shutdown() {
    close(fd);
}

bool is_root() {
    return (getuid() == 0) || (geteuid() == 0);
}