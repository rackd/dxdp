#include "dxdp-driver/argp.h"
#include "dxdp-ipc/ipc.h"
#include "dxdp-ipc/ipc_message.h"
#include "dxdp/sysutil.h"
#include "dxdp/string_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    ipc_packet* packet = ipc_new_packet();
    p_args args;
    if(!parse_args(argc, argv, &args)) {
        return 1;
    }

    switch(args.command) {
    case COMMAND_UPDATE:
        switch (args.option) {
        case OPTION_ADD_IP_STR:
        case OPTION_REMOVE_IP_STR:
            if(args.option == OPTION_ADD_IP_STR) {
                packet->message = IPC_MESSAGE_ADD_IP;
            } else {
                packet->message = IPC_MESSAGE_REMOVE_IP;
            }

            strcpy(packet->argv[0], args.section_id);
            if(args.is_ipv4) {
                strcpy(packet->argv[1], "4");
            } else {
                strcpy(packet->argv[1], "6");
            }

            size_t c = 2;
            for(size_t i = 0; i < args.num_ips; i++) {
                strncpy(packet->argv[c++], args.ips[i].low, MAX_ARG_LENGTH);
                printf("%s\n", args.ips[i].low);
                printf("%s\n", args.ips[i].high);
                strncpy(packet->argv[c++], args.ips[i].high, MAX_ARG_LENGTH);
            }

            packet->argc = (args.num_ips * 2) + 2;
        break;

        case OPTION_ADD_IP_FILE:
        case OPTION_REMOVE_IP_FILE:
            if(args.option == OPTION_ADD_IP_FILE) {
                packet->message = IPC_MESSAGE_ADD_IP_FILE;
            } else {
                packet->message = IPC_MESSAGE_REMOVE_IP_FILE;
            }

            strncpy(packet->argv[0], args.section_id, MAX_ARG_LENGTH);
            if(args.is_ipv4) {
                strcpy(packet->argv[1], "4");
            } else {
                strcpy(packet->argv[1], "6");
            }
            strncpy(packet->argv[2], args.ips_file, MAX_ARG_LENGTH);
            packet->argc = 3;
        break;

        default:
        return 1;
        break;
        }
    break;

    case COMMAND_STATUS:
        packet->message = IPC_MESSAGE_REQUEST_STATUS;
        packet->argc = 0;
    break;

    case COMMAND_DETACH:
        packet->message = IPC_MESSAGE_DETACH;
        strncpy(packet->argv[0], args.section_id, MAX_ARG_LENGTH);
        packet->argc = 1;
    break;

    case COMMAND_QUIT:
        packet->argc = 0;
        packet->message = IPC_MESSAGE_QUIT;
    break;

    case COMMAND_GEO_DB_UPDATE:
        fprintf(stdout, "Run 'dxdp-db-update' to update the geolocation IP database.\n");
    break;

    default:
    return 1;
    break;
    }

    ipc_srv* srv = ipc_cli_connect(IPC_SHM_NAME);
    if(srv == NULL) {
        fprintf(stderr, "Failed to connect...\n");
        return 0;
    }

    ipc_packet* response_packet = ipc_new_packet();

    bool res = ipc_cli_send(srv, packet, response_packet);
    if(!res) {
        fprintf(stderr, "Failed to send...\n");
    }

    if(args.command == COMMAND_STATUS) {
        for(size_t i = 0; i < response_packet->argc; i++) {
            status_packet* sp = (status_packet*) response_packet->argv[i];
            fprintf(stdout, "Interface name: %s\n", sp->if_name);
            fprintf(stdout, "\tIndex: %i\n", sp->if_index);
            fprintf(stdout, "\tMode: %u\n", sp->mode);
            fprintf(stdout, "\tNumber of IPV4 entries: %zu\n", sp->num_ipv4s);
            fprintf(stdout, "\tNumber of IPV6 entries: %zu\n", sp->num_ipv6s);
        }
    } else {
        char buffer[1024];
        ipc_response_2_str(response_packet->message, buffer);
        fprintf(stdout, "%s\n", buffer);
    }

    free(packet);
    free(response_packet);
    return 0;
}