#include "dxdp-ipc/ipc.h"
#include "dxdp/ipc_action.h"
#include "dxdp/sysutil.h"
#include "dxdp/string_util.h"
#include "dxdp/bpf_handler.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(ipc_packet* packet, void* addl_data) {
    handler_context* context = (handler_context*)addl_data;
    ipc_packet* response = context->response_packet;
    void* a_data = context->addl_data;
    ldata* data = (ldata*) a_data;
    ipc_srv* srv = context->srv;

    response->argc = 0;
    response->message = IPC_MESSAGE_NONE;

    switch(packet->message) {
        case IPC_MESSAGE_ADD_IP:
        case IPC_MESSAGE_REMOVE_IP:
            if(!ENSURE_MIN_ARG(packet, 2)) {
                response->message = IPC_MESSAGE_INVALID_IP;
                break;
            };

            ip_type type;
            if(strcmp(packet->argv[1], "4") == 0) {
               type = IP_TYPE_IPV4;
            } else {
                type = IP_TYPE_IPV6;
            }

            ip_action action;
            if (packet->message == IPC_MESSAGE_ADD_IP) {
                action = IPA_ADD;
            } else {
                action = IPA_REMOVE;
            }

            response->message = ip_task_mem(type, action, packet->argv[0],
                packet->argv, packet->argc-1, data);
            break;

        case IPC_MESSAGE_ADD_IP_FILE:
            if(!ENSURE_MIN_ARG(packet, 2)) {
                response->message = IPC_MESSAGE_INVALID_IP;
                break;
            };
            ip_type type2;
            if(strcmp(packet->argv[1], "4") == 0) {
               type2 = IP_TYPE_IPV4;
            } else {
                type2= IP_TYPE_IPV6;
            }

            response->message = ip_task_file(type2, IPA_ADD, packet->argv[0],
                packet->argv[2], data);
            break;

        case IPC_MESSAGE_REQUEST_STATUS:
            ipc_packet* ipc_sp = get_status(data);
            if(ipc_sp != NULL) {
                memcpy(response, ipc_sp, sizeof(ipc_packet));
                free(ipc_sp);
            }
        break;

        case IPC_MESSAGE_DETACH:
            if(!ENSURE_MIN_ARG(packet, 1)) {
                response->message = IPC_MESSAGE_INVALID_IP;
                break;
            };

            uint32_t if_index;
            if(su_str_2_uint32(packet->argv[0], &if_index)) {
                eebpf_detach_force(if_index);
                response->message = IPC_MESSAGE_SUCCESS;
            } else {
                response->message = IPC_MESSAGE_ERROR;
            }

        break;

        case IPC_MESSAGE_QUIT:
        response->message = IPC_MESSAGE_SUCCESS;
        srv->enabled = false;
        break;

        default:
            break;
    }
}

ipc_srv* ipc_init(ldata* ld) {
    void* additional_data = (void*) ld;
    ipc_srv* server = ipc_srv_init(IPC_SHM_NAME, handler, additional_data,
        IPC_GRP_NAME);
        ld->srv = server;
    if (!server) {
        return NULL;
    }
    return server;
}
