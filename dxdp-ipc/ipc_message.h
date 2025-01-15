#ifndef IPC_MESSAGE_H_
#define IPC_MESSAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IPC_MESSAGE_SUCCESS                     = 1,
    IPC_MESSAGE_ERROR                       = 2,
    IPC_MESSAGE_NONE                        = 3,
    IPC_MESSAGE_SECTION_MUST_BE_GEO         = 4,
    IPC_MESSAGE_SECTION_MUST_BE_RANGE       = 5,
    IPC_MESSAGE_SECTION_DOES_NOT_EXIST    = 6,
    IPC_MESSAGE_INVALID_IP                  = 7,
    IPC_MESSAGE_INVALID_ARGS                = 8
} ipc_message_response;

typedef enum {
    IPC_MESSAGE_ADD_IP                      = 9,
    IPC_MESSAGE_ADD_IP_FILE                 = 10,
    IPC_MESSAGE_REMOVE_IP                   = 11,
    IPC_MESSAGE_REMOVE_IP_FILE              = 12,
    IPC_MESSAGE_DETACH                      = 13,
    IPC_MESSAGE_QUIT                        = 14,
    IPC_MESSAGE_REQUEST_STATUS              = 15
} ipc_message;

typedef enum {
    IPC_MESSAGE_COMPILE_ME                  = 16,
} ipc_comp_message;

bool ipc_response_2_str(ipc_message_response message, char* _buff_dest);

#ifdef __cplusplus
}
#endif

#endif // IPC_MESSAGE_H_