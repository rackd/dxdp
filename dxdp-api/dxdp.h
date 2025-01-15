#ifndef DXDP_H_
#define DXDP_H_

#include "dxdp/if_handler.h"

typedef struct ipv4_range {
    uint32_t low;
    uint32_t high;
} ipv4_range;

typedef struct ipv6_range {
    uint8_t low[16];
    uint8_t high[16];
} ipv6_range;

void dxdp_init();
int dxdp_add_ipv4(ipv4_range* _rng, const char* _section_name);
int dxdp_add_ipv6(ipv6_range* _rng, const char* _section_name);
int dxdp_rm_ipv4(ipv4_range* _rng, const char* _section_name);
int dxdp_rm_ipv6(ipv6_range* _rng, const char* _section_name);

#endif // DXDP_H_