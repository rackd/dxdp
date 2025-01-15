#include "dxdp-ipc/ipc_message.h"
#include <string.h>

bool ipc_response_2_str(ipc_message_response message, char* _buff_dest) {
    if (!_buff_dest) return false;

    switch (message) {
        case IPC_MESSAGE_SUCCESS:
            strcpy(_buff_dest, "Success");
            break;
        case IPC_MESSAGE_ERROR:
            strcpy(_buff_dest, "Error");
            break;
        case IPC_MESSAGE_NONE:
            strcpy(_buff_dest, "None");
            break;
        case IPC_MESSAGE_SECTION_MUST_BE_GEO:
            strcpy(_buff_dest, "Section must be GEO");
            break;
        case IPC_MESSAGE_SECTION_MUST_BE_RANGE:
            strcpy(_buff_dest, "Section must be range");
            break;
        case IPC_MESSAGE_SECTION_DOES_NOT_EXIST:
            strcpy(_buff_dest, "Section does not exist");
            break;
        case IPC_MESSAGE_INVALID_IP:
            strcpy(_buff_dest, "Invalid IP");
            break;
        case IPC_MESSAGE_INVALID_ARGS:
            strcpy(_buff_dest, "Invalid args");
            break;
        default:
            strcpy(_buff_dest, "Unknown message");
            return false;
    }
    return true;
}