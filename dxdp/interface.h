#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#define MAX_INDEX 1024
#define MAX_INTERFACES 1024

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

typedef struct interface {
    uint32_t ipv4;
    uint8_t ipv6[16];
    uint32_t ipv4_subnet;
    uint32_t ipv4_subnet_mask;
    uint8_t mac[6];
    sa_family_t family;
} interface;

typedef struct if_index {
    char name[IFNAMSIZ];
    uint32_t index;
    bool xdp_cap;
    interface* interfaces;
    size_t num_interfaces;
} if_index;

/* Get system interfaces */
if_index* if_get_interfaces(size_t* _num_if_indexes);

void if_free(if_index* _if_indexes, size_t _num_indexes);

#endif //INTERFACE_H_