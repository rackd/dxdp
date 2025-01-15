#ifndef IF_HANDLER_H_
#define IF_HANDLER_H_

#define MAC_ADDRSTRLEN 17
#define MAX_MATCHES 20
#define IFH_CACHE_LINE_BUFFER_LEN 128

#include "dxdp/interface.h"
#include "dxdp/config.h"
#include "dxdp/db.h"
#include "dxdp/itree_uint32.h"
#include <stdbool.h>

typedef struct ipv4_range{
    uint32_t low;
    uint32_t high;
} ipv4_range;

typedef struct ipv6_range {
    uint8_t low[16];
    uint8_t high[16];
} ipv6_range;

typedef struct if_h {
    uint32_t if_index;
    size_t priority;
    section_mode mode;
    ipv4_range* ipv4s;
    size_t num_ipv4s;
    ipv6_range* ipv6s;
    size_t num_ipv6s;
} if_h;

/* Search for matching interfaces mentioned in a config section given
 * interfaces array and pointer to a config section.
 */
if_index** ifh_match_if(const if_index* _if_indexes,
    size_t _num_if_indexes, config_section* p_section,
    size_t* _num_indexes_matches);

/* Create if handles given parsed config sections */
if_h* ifh_create(config_psd_section* _psd_sections,
    size_t _num_sections, size_t* _num_ifhs);

void ifh_free(if_h* _ifhs, size_t _num_ifhs);

/* Given an array of if handlers, ensure its IPV4 and IPV6 ranges are valid*/
bool ifh_make_valid(if_h* _ifhs, size_t _num_ifhs);

/* Get MAC addr of _if_index and write to _dest */
bool ifh_if_index_2_mac(uint32_t _if_index, uint8_t* _dest,
    const if_index* _if_indexes, size_t _num_if_indexes);

/* Get _if_index of mac address and write to _dest */
bool ifh_mac_2_if_index(uint8_t mac[6], uint32_t* _dest,
    const if_index* _if_indexes, size_t _num_if_indexes);

/* Merge two if handles on if_index and mode (whitelist/blacklist)*/
if_h* ifh_merge(const if_h* _ifh1,
    size_t _ifh1_num_ifhs, const if_h* _ifh2,
    size_t _ifh2_num_ifhs, size_t* _out_num_ifhs);

/* Remove all ranges in _rm_rngs array of ranges from *_in_rngs_ptr
 * reallocating the in array.
 */
void ifh_rm_rngs_ipv4(
    ipv4_range** _in_rngs_ptr, size_t _num_in_rngs,
    ipv4_range* _rm_rngs, size_t _num_rm_rngs,
    size_t* _out_num_rngs);

/* Remove all ranges in _rm_rngs array of ranges from *_in_rngs_ptr
 * reallocating the in array.
 */
void ifh_rm_rngs_ipv6(
    ipv6_range** _in_rngs_ptr, size_t _num_in_rngs,
    ipv6_range* _rm_rngs, size_t _num_rm_rngs,
    size_t* _out_num_rngs);

/* Get all unique if_indexes of a _ifh array*/
uint32_t* ifh_get_uniq_idx(if_h* _ifhs, size_t _num_fhs, size_t* _num_idxs);

/* Write _index interface name to _buff_out */
bool ifh_i_2_name(uint32_t _index, const if_index* _if_indexes,
    size_t _num_if_indexes, char* _buff_out);

/* Take an array of if handlers, then, adjust it given demands
 * from CLI DB.
 */
int ifh_root_add_cli_db(if_h** p_ifhs, size_t _num_ifhs,
    config_psd_section* _sections, size_t _num_section);

#endif // IF_HANDLER_H_
