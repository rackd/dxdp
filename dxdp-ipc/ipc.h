#ifndef IPC_SOCKET_H_
#define IPC_SOCKET_H_

#ifdef __cplusplus
    #include <atomic>
    typedef std::atomic<bool> atomic_bool;
    typedef std::atomic<uint32_t> atomic_uint32_t;
#else
    #include <stdatomic.h>
    typedef atomic_bool atomic_bool;
    typedef atomic_uint atomic_uint32_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/ipc.h>

#define MAX_ARG_COUNT 10
#define MAX_ARG_LENGTH 5000000
#define ENSURE_MIN_ARG(packet, n) ((packet)->argc >= (n))
#define MAX_QUEUE_PACKETS 10
#define SERVER_TIMEOUT 5 

typedef struct ipc_packet {
    uint32_t message;
    uint32_t argc;
    char argv[MAX_ARG_COUNT][MAX_ARG_LENGTH];
} ipc_packet;

typedef struct status_packet {
    char if_name[IFNAMSIZ];
    uint32_t if_index;
    uint32_t mode;
    size_t num_ipv4s;
    size_t num_ipv6s;
} status_packet;

typedef void (*packet_handler)(ipc_packet* client_packet, void* addl_data);

typedef struct mem_map {
    /* A bool indicating weather a client spot is open */
    atomic_bool free_map[MAX_QUEUE_PACKETS];

    /* A bool indicating weather client is done writing */
    atomic_bool cli_done_map[MAX_QUEUE_PACKETS];

    /* A bool indicating weather client is done reading response */
    atomic_bool cli_done_reading_map[MAX_QUEUE_PACKETS];

    /* A bool indicating weather server is done reading and writing */
    atomic_bool srv_done_map[MAX_QUEUE_PACKETS];

    /* Client should write here */
    ipc_packet packet_in[MAX_QUEUE_PACKETS];

    /* Server should write response here */
    ipc_packet packet_response[MAX_QUEUE_PACKETS];

    /* This contains unix timestamp when server read the packet */
    time_t watchdog_map[MAX_QUEUE_PACKETS];

    /* Client writes here for syncing */
    uint32_t client_nonce[MAX_QUEUE_PACKETS];
} mem_map;

typedef struct ipc_srv {
    int shm_fd;
    char* name;
    mem_map* data;
    bool enabled;
    pthread_t thread;
    packet_handler handler;
    void* addl_data;
} ipc_srv;

typedef struct handler_context {
    ipc_packet* response_packet;
    void* addl_data;
    ipc_srv* srv;
} handler_context;

ipc_packet* ipc_new_packet();

void ipc_packet_free(ipc_packet* packet);

ipc_srv* ipc_srv_init(const char* _name, packet_handler handler, void* addl_data, const char* _group);

void ipc_srv_destroy(ipc_srv* _socket);

ipc_srv* ipc_cli_connect(const char* _name);

bool ipc_cli_send(ipc_srv* client, ipc_packet* _packet, ipc_packet* _response) ;

#ifdef __cplusplus
}

#endif
#endif // IPC_SOCKET_H_