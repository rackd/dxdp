#include "dxdp/interface.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

if_index* if_get_interfaces(size_t *_num_if_indexes) {
    struct ifaddrs *addrs;
    if (getifaddrs(&addrs) != 0) {
        return NULL;
    }

    int occurrences[MAX_INDEX];
    int index_map[65536];
    int unique_count = 0;

    memset(index_map, -1, sizeof(index_map));

    struct ifaddrs *tmp = addrs;
    while (tmp) {
        unsigned int ifindex = if_nametoindex(tmp->ifa_name);

        if (index_map[ifindex] == -1) {
            index_map[ifindex] = unique_count;
            occurrences[unique_count] = 1;
            unique_count++;
        } else {
            occurrences[index_map[ifindex]]++;
        }

        tmp = tmp->ifa_next;
    }

    if_index* if_indexes = malloc(sizeof(if_index) * unique_count);
    if (!if_indexes) {
        freeifaddrs(addrs);
        return NULL;
    }

    for(int i = 0; i < unique_count; i++) {
        if_indexes[i].interfaces = malloc(sizeof(interface) * occurrences[i]);
        if(!if_indexes[i].interfaces) {
            freeifaddrs(addrs);
            return NULL;
        }

        if_indexes[i].num_interfaces = 0;
    }

    tmp = addrs;
    while (tmp) {
        unsigned int ifindex = if_nametoindex(tmp->ifa_name);
        int seq_index = index_map[ifindex];

        if_index* _if_index = &if_indexes[seq_index];
        strcpy(_if_index->name, tmp->ifa_name);

        _if_index->index = ifindex;
        _if_index->xdp_cap = false;

        interface _interface;
        _interface.family = AF_UNSPEC;
        _interface.ipv4 = 0;
        _interface.ipv4_subnet = 0;
        memset(_interface.ipv6, 0, sizeof(uint8_t) * 16);
        memset(_interface.mac, 0, sizeof(uint8_t) * 6);

        if (tmp->ifa_addr) {
            _interface.family = tmp->ifa_addr->sa_family;

            if (tmp->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)tmp->ifa_addr;
                 _interface.ipv4 = ipv4->sin_addr.s_addr;

                if (tmp->ifa_netmask) {
                    struct sockaddr_in* netmask =
                        (struct sockaddr_in *)tmp->ifa_netmask;
                    _interface.ipv4_subnet_mask =
                        netmask->sin_addr.s_addr;
                    _interface.ipv4_subnet =
                        ipv4->sin_addr.s_addr & netmask->sin_addr.s_addr;
                }
            } else if (tmp->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6* ipv6 =
                    (struct sockaddr_in6 *)tmp->ifa_addr;
                memcpy(_interface.ipv6, &ipv6->sin6_addr,
                    sizeof(struct in6_addr));
            } else if (tmp->ifa_addr->sa_family == AF_PACKET) {
                struct sockaddr_ll* sll = (struct sockaddr_ll *)tmp->ifa_addr;
                memcpy(_interface.mac, sll->sll_addr, 6);
            }
        }

         _if_index->interfaces[_if_index->num_interfaces++] = _interface;
         tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    *_num_if_indexes = unique_count;

    return if_indexes;
}

void if_free(if_index* _if_indexes, size_t _num_indexes) {
    for (size_t i = 0; i < _num_indexes; i++) {
        free(_if_indexes[i].interfaces);
    }
    free(_if_indexes);
}