#include "dxdp/proc_comp.h"
#include "dxdp/sysutil.h"
#include "dxdp/proc.h"
#include "dxdp-ipc/ipc.h"
#include "dxdp-ipc/ipc_message.h"
#include "dxdp/options.h"
#include "dxdp/log.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

proc_h* pc_spawn() {
    log_write(LL_INFO, "Using compiler path: %s\n", COMP_PATH);
    const char* argv[] = { "compiler"};
    proc_h* handle = proc_init(COMP_PATH, 1, argv);
    if(!handle) {
        return NULL;
    }
    return handle;
}

bool pc_comp(const char* _in_source, size_t _in_size,
char* _out_buff, size_t* _bin_size) {
    ipc_srv* srv = ipc_cli_connect(IPC_SHM_CIMP_NAME);
    if(!srv) {
        return false;
    }

    ipc_packet* packet = malloc(sizeof(ipc_packet));
    packet->argc = 1;
    packet->message = IPC_MESSAGE_COMPILE_ME;

    if(_in_size > MAX_ARG_COUNT * MAX_ARG_LENGTH) {
        log_write(LL_ERROR, "Dev error: C prog too big!\n");
        return false;
    }

    memcpy(packet->argv[0], _in_source, _in_size);

    ipc_packet* response = malloc(sizeof(ipc_packet));
    if(!ipc_cli_send(srv, packet, response)) {
        free(packet);
        free(response);
        log_write(LL_ERROR, "COMP: Failed to send packet.\n");
        return false;
    }

    if(response->message != IPC_MESSAGE_SUCCESS) {
        free(packet);
        free(response);
        log_write(LL_ERROR, "COMP: Message is not success.\n");
        return false;
    }

    memcpy(_bin_size, response->argv[1], sizeof(size_t));
    if(*_bin_size > (MAX_ARG_COUNT * MAX_ARG_LENGTH)) {
        free(packet);
        free(response);
        log_write(LL_ERROR, "COMP: BPF prog too big\n");
        return false;
    }

    memcpy(_out_buff, response->argv[0], *_bin_size);

    free(packet);
    free(response);
    ipc_srv_destroy(srv);
    return true;
}

void ps_spawn_free(proc_h* handle) {
    free(handle);
}