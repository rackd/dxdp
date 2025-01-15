#ifndef IPC_ACTION_H_
#define IPC_ACTION_H_

#include "dxdp-ipc/ipc.h"
#include "dxdp-ipc/ipc_message.h"
#include "dxdp/if_handler.h"
#include <stdint.h>
#include <stddef.h>

typedef struct ldata {
    config_psd_section* psd_sections;
    size_t num_psd_sections;
    const if_index* if_indexes;
    size_t num_if_indexes;
    if_h** root_ifh;
    size_t* num_root_ifhs;
    ipc_srv* srv;
} ldata;

typedef enum ip_type {
    IP_TYPE_IPV4 = 0,
    IP_TYPE_IPV6 = 1
} ip_type;

typedef enum ip_action {
    IPA_ADD=0,
    IPA_REMOVE=1
}ip_action;

/* Init IPC server*/
ipc_srv* ipc_init(ldata* ld);

/* IPC handler function: do action based on request. */
ipc_message ip_task_mem(ip_type type, ip_action action,
    const char* _section_name, const char _argv[][MAX_ARG_LENGTH],
    size_t _argc, ldata* data);

/* IPC handler function: do action based on request. */
ipc_message ip_task_file(ip_type type, ip_action action,
    const char* _section_name, const char* _path, ldata* data);

/* Get status packet */
ipc_packet* get_status(ldata* data);

#endif // IPC_ACTION_H_