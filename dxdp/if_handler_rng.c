#include "dxdp/if_handler.h"
#include "dxdp/db.h"
#include "dxdp/hash_map.h"
#include "dxdp/cantor_hash.h"
#include "dxdp/cli_db.h"
#include "dxdp/sysutil.h"
#include "dxdp/mem_util.h"
#include "dxdp/options.h"
#include "dxdp/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

if_h* ifh_create(config_psd_section* _psd_sections,
size_t _num_sections, size_t* _num_ifhs) {
    size_t sum_interfaces_matched = 0;
    for(size_t i = 0; i < _num_sections; i++) {
        sum_interfaces_matched += _psd_sections[i].num_interfaces_matched;
    }

    if_h* ifhs = malloc(sizeof(if_h) * sum_interfaces_matched);
    hash_map* hm = hm_new(HM_INIT_SIZE_DEFAULT);
    size_t num_ifhs = 0;

    for(size_t i = 0; i < _num_sections; i++) {
        db_s* db = db_init();
        if(!db_read(db, &_psd_sections[i])) {
            return NULL;
        }

        for(size_t y = 0; y < _psd_sections[i].num_interfaces_matched; y++) {
            size_t key =
                cantor_pairing_hash(
                    _psd_sections[i].matched_interfaces[y]->index,
                    _psd_sections[i].mode);
            size_t value;

            if(!hm_exists(hm, key)) {
                value = num_ifhs;
                ifhs[value].if_index =
                    _psd_sections[i].matched_interfaces[y]->index;
                ifhs[value].mode = _psd_sections[i].mode;
                ifhs[value].ipv4s = NULL;
                ifhs[value].ipv6s = NULL;
                ifhs[value].num_ipv4s = 0;
                ifhs[value].num_ipv6s = 0;
                hm_insert(hm, key, num_ifhs++);
            } else {
                value = hm_value_for(hm, key);
            }

            if(db->num_ipv4 > 0) {
                for(size_t z = 0; z < db->num_ipv4; z++) {
                    ipv4_range rng;
                    rng.low = db->ipv4s[z].low;
                    rng.high = db->ipv4s[z].high;

                    ipv4_range* tmp = realloc(ifhs[value].ipv4s,
                        sizeof(ipv4_range) * ++ifhs[value].num_ipv4s);
                    if(!tmp) {
                        free(ifhs);
                        return NULL;
                    }
                    ifhs[value].ipv4s = tmp;
                    ifhs[value].ipv4s[ifhs[value].num_ipv4s - 1] = rng;
                }
            }

            if(db->num_ipv6 > 0) {
                for(size_t z = 0; z < db->num_ipv6; z++) {
                    ipv6_range rng;
                    memcpy(rng.low, db->ipv6s[z].low, sizeof(rng.low));
                    memcpy(rng.high, db->ipv6s[z].high, sizeof(rng.high));

                    ipv6_range* tmp = realloc(ifhs[value].ipv6s,
                        sizeof(ipv6_range) * ++ifhs[value].num_ipv6s);
                    if(!tmp) {
                        free(ifhs);
                        return NULL;
                    }
                    ifhs[value].ipv6s = tmp;
                    ifhs[value].ipv6s[ifhs[value].num_ipv6s - 1] = rng;
                }
            }
        }

        db_free(db);
    }

    hm_free(hm);
    *_num_ifhs = num_ifhs;
    return ifhs;
}

void ifh_free(if_h* _ifhs, size_t _num_ifhs) {
    for(size_t i = 0; i < _num_ifhs; i++) {
        if(_ifhs[i].num_ipv4s > 0) {
            free(_ifhs[i].ipv4s);
        }

        if(_ifhs[i].num_ipv6s > 0) {
            free(_ifhs[i].ipv6s);
        }
    }

    free(_ifhs);
}

int _compare_ipv4(const void* a, const void* b) {
    const ipv4_range* rng_a = (const ipv4_range*)a;
    const ipv4_range* rng_b = (const ipv4_range*)b;
    if (rng_a->low < rng_b->low) return -1;
    if (rng_a->low > rng_b->low) return 1;
    return 0;
}

int _compare_ipv6(const void* a, const void* b) {
    const ipv6_range* rng_a = (const ipv6_range*)a;
    const ipv6_range* rng_b = (const ipv6_range*)b;
    return memcmp(rng_a->low, rng_b->low, 16);
}

bool _ifh_fix_overlap_ipv4(ipv4_range* _rngs, size_t _num_rngs,
size_t* _out_num_rngs) {
    qsort(_rngs, _num_rngs, sizeof(ipv4_range), _compare_ipv4);

    size_t write_index = 0;

    for (size_t read_index = 1; read_index < _num_rngs; read_index++) {
        ipv4_range* last = &_rngs[write_index];
        ipv4_range* current = &_rngs[read_index];

        if (current->low <= last->high + 1) {
            if (current->high > last->high) {
                last->high = current->high;
            }
        } else {
            write_index++;
            _rngs[write_index] = _rngs[read_index];
        }
    }

    *_out_num_rngs = write_index + 1;
    return true;
}

bool _ifh_fix_overlap_ipv6(ipv6_range* _rngs,
size_t _num_rngs, size_t* _out_num_rngs) {
    qsort(_rngs, _num_rngs, sizeof(ipv6_range), _compare_ipv6);

    size_t write_index = 0;

    for (size_t read_index = 1; read_index < _num_rngs; read_index++) {
        ipv6_range* last = &_rngs[write_index];
        ipv6_range* current = &_rngs[read_index];

        __uint128_t l_high = u8_2_128(last->high);
        __uint128_t c_low = u8_2_128(current->low);
        __uint128_t c_high = u8_2_128(current->high);


        if (c_low <= l_high + 1) {
            if (c_high > l_high) {
                memcpy(last->high, current->high, sizeof(last->high));
            }
        } else {
            write_index++;
            _rngs[write_index] = _rngs[read_index];
        }
    }

    *_out_num_rngs = write_index + 1;
    return true;
}

//TODO: Use memcmp instead of __uint128_t
void ifh_rm_rngs_ipv4(
ipv4_range** _in_rngs_ptr, size_t _num_in_rngs,
ipv4_range* _rm_rngs, size_t _num_rm_rngs,
size_t* _out_num_rngs) {
    ipv4_range* _in_ifhs = *_in_rngs_ptr;

    qsort(_rm_rngs, _num_rm_rngs, sizeof(ipv4_range), _compare_ipv4);

    size_t temp_size = _num_in_rngs + _num_rm_rngs * 2;
    ipv4_range* temp_ifh = malloc(sizeof(ipv4_range) * temp_size);
    if (temp_ifh == NULL) {
        return;
    }

    size_t out_count = 0;
    size_t rm_idx = 0;

    for (size_t in_idx = 0; in_idx < _num_in_rngs; ++in_idx) {
        ipv4_range current = _in_ifhs[in_idx];
        uint32_t current_start = current.low;
        uint32_t current_end = current.high;

        while (rm_idx < _num_rm_rngs && _rm_rngs[rm_idx].high < current_start) {
            rm_idx++;
        }

        size_t temp_rm_idx = rm_idx;

        while (temp_rm_idx < _num_rm_rngs
        && _rm_rngs[temp_rm_idx].low <= current_end) {
            ipv4_range removal = _rm_rngs[temp_rm_idx];

            if (removal.low > current_end) {
                break;
            }

            if (removal.low > current_start) {
                temp_ifh[out_count].low = current_start;
                temp_ifh[out_count].high = removal.low - 1;
                out_count++;
            }

            if (removal.high + 1 > current_start) {
                current_start = removal.high + 1;
            }

            if (current_start > current_end) {
                break;
            }

            temp_rm_idx++;
        }

        if (current_start <= current_end) {
            temp_ifh[out_count].low = current_start;
            temp_ifh[out_count].high = current_end;
            out_count++;
        }
    }

    ipv4_range* resized_ifhs = realloc(_in_ifhs,
        sizeof(ipv4_range) * out_count);
    if (resized_ifhs == NULL) {
        free(temp_ifh);
        return;
    }

    *_in_rngs_ptr = resized_ifhs;
    *_out_num_rngs = out_count;

    for (size_t i = 0; i < out_count; ++i) {
        resized_ifhs[i] = temp_ifh[i];
    }

    free(temp_ifh);
}

//TODO: Use memcmp instead of __uint128_t
void ifh_rm_rngs_ipv6(
ipv6_range** _in_rngs_ptr, size_t _num_in_rngs,
ipv6_range* _rm_rngs, size_t _num_rm_rngs,
size_t* _out_num_rngs){
    ipv6_range* _in_ifhs = *_in_rngs_ptr;

    qsort(_rm_rngs, _num_rm_rngs, sizeof(ipv6_range), _compare_ipv6);

    size_t temp_size = _num_in_rngs + _num_rm_rngs * 2;
    ipv6_range* temp_ifh = malloc(sizeof(ipv6_range) * temp_size);
    if (temp_ifh == NULL) {
        return;
    }

    size_t out_count = 0;
    size_t rm_idx = 0;

    for (size_t in_idx = 0; in_idx < _num_in_rngs; ++in_idx) {
        ipv6_range current = _in_ifhs[in_idx];
        __uint128_t current_start = u8_2_128(current.low);
        __uint128_t current_end = u8_2_128(current.high);
        __uint128_t rm_rng_high = u8_2_128(_rm_rngs[rm_idx].high);

        while (rm_idx < _num_rm_rngs && rm_rng_high < current_start) {
            rm_idx++;
        }

        size_t temp_rm_idx = rm_idx;
        __uint128_t rm_low = u8_2_128(_rm_rngs[temp_rm_idx].low);

        while (temp_rm_idx < _num_rm_rngs && rm_low <= current_end) {
            ipv6_range removal = _rm_rngs[temp_rm_idx];
            __uint128_t rmvl_low = u8_2_128(removal.low);
            __uint128_t rmvl_high = u8_2_128(removal.high);

            if (rmvl_low > current_end) {
                break;
            }

            if (rmvl_low > current_start) {
                memcpy(temp_ifh[out_count].low, &current_start, sizeof(current_start));
                memcpy(temp_ifh[out_count].high, &rmvl_low, sizeof(rmvl_low));
                out_count++;
            }

            if (rmvl_high + 1 > current_start) {
                current_start = rmvl_high + 1;
            }

            if (current_start > current_end) {
                break;
            }

            temp_rm_idx++;
        }

        if (current_start <= current_end) {
            memcpy(temp_ifh[out_count].low, &current_start, sizeof(temp_ifh[out_count].low));
            memcpy(temp_ifh[out_count].high, &current_end, sizeof(temp_ifh[out_count].high));
            out_count++;
        }
    }

    ipv6_range* resized_ifhs = realloc(_in_ifhs,
        sizeof(ipv6_range) * out_count);
    if (resized_ifhs == NULL) {
        free(temp_ifh);
        return;
    }

    *_in_rngs_ptr = resized_ifhs;
    *_out_num_rngs = out_count;

    for (size_t i = 0; i < out_count; ++i) {
        resized_ifhs[i] = temp_ifh[i];
    }

    free(temp_ifh);
}

bool ifh_make_valid(if_h* _ifhs, size_t _num_ifhs) {
    for(size_t i = 0; i < _num_ifhs; i++) {
        if(_ifhs[i].num_ipv4s > 0) {
            if(!_ifh_fix_overlap_ipv4(_ifhs[i].ipv4s, _ifhs[i].num_ipv4s,
            &_ifhs[i].num_ipv4s)) {
                 return false;
            }
        }

        if(_ifhs[i].num_ipv6s > 0) {
            if(!_ifh_fix_overlap_ipv6(_ifhs[i].ipv6s, _ifhs[i].num_ipv6s,
            &_ifhs[i].num_ipv6s)) {
                return false;
            }
        }
    }

    return true;
}

if_h* ifh_merge(const if_h* _ifh1,
size_t _ifh1_num_ifhs, const if_h* _ifh2,
size_t _ifh2_num_ifhs, size_t* _out_num_ifhs) {
    size_t sum_ifhs = _ifh1_num_ifhs + _ifh2_num_ifhs;
    if_h* merged_ifhs = malloc(sizeof(if_h) * sum_ifhs);
    if (!merged_ifhs) {
        return NULL;
    }

    hash_map* hm = hm_new(HM_INIT_SIZE_DEFAULT);
    hash_map* ips_hm_ipv4 = hm_new(HM_INIT_SIZE_DEFAULT);
    hash_map* ips_hm_ipv6 = hm_new(HM_INIT_SIZE_DEFAULT);

    for(size_t i = 0; i < _ifh1_num_ifhs; i++) {
        size_t key = cantor_pairing_hash(_ifh1[i].if_index, _ifh1[i].mode);

        if(!hm_exists(hm, key)) {
            hm_insert(hm, key, 1234);
            hm_insert(ips_hm_ipv4, key, 0);
            hm_insert(ips_hm_ipv6, key, 0);
        }

        size_t num_ipv4 = hm_value_for(ips_hm_ipv4, key);
        num_ipv4 += _ifh1[i].num_ipv4s;
        hm_insert(ips_hm_ipv4, key, num_ipv4);

        size_t num_ipv6 = hm_value_for(ips_hm_ipv6, key);
        num_ipv6 += _ifh1[i].num_ipv6s;
        hm_insert(ips_hm_ipv6, key, num_ipv6);
    }

    for(size_t i = 0; i < _ifh1_num_ifhs; i++) {
        size_t key = cantor_pairing_hash(_ifh2[i].if_index, _ifh2[i].mode);

        if(!hm_exists(hm, key)) {
            hm_insert(hm, key, 1234);
            hm_insert(ips_hm_ipv4, key, 0);
            hm_insert(ips_hm_ipv6, key, 0);
        }

        size_t num_ipv4 = hm_value_for(ips_hm_ipv4, key);
        num_ipv4 += _ifh2[i].num_ipv4s;
        hm_insert(ips_hm_ipv4, key, num_ipv4);

        size_t num_ipv6 = hm_value_for(ips_hm_ipv6, key);
        num_ipv6 += _ifh2[i].num_ipv6s;
        hm_insert(ips_hm_ipv6, key, num_ipv6);
    }

    hm_free(hm);
    hm = hm_new(HM_INIT_SIZE_DEFAULT);
    size_t idx = 0;
    size_t c = 0;
    for(size_t i = 0; i < _ifh1_num_ifhs; i++) {
        size_t key = cantor_pairing_hash(_ifh1[i].if_index, _ifh1[i].mode);

        if(!hm_exists(hm, key)) {
            idx = c;
            hm_insert(hm, key, c++);

            merged_ifhs[idx].num_ipv4s = 0;
            merged_ifhs[idx].num_ipv6s = 0;

            size_t num_ipv4s = hm_value_for(ips_hm_ipv4, key);
            if(num_ipv4s > 0) {
                merged_ifhs[idx].ipv4s =
                    malloc(sizeof(ipv4_range) * num_ipv4s);
                if(!merged_ifhs[idx].ipv4s) {
                    return NULL;
                }
            }

            size_t num_ipv6s = hm_value_for(ips_hm_ipv6, key);
            if(num_ipv6s > 0) {
                merged_ifhs[idx].ipv6s =
                    malloc(sizeof(ipv6_range) * num_ipv6s);
                if(!merged_ifhs[idx].ipv6s) {
                    return NULL;
                }
            }

            merged_ifhs[idx].if_index = _ifh1[i].if_index;
            merged_ifhs[idx].mode = _ifh1[i].mode;
        } else {
            idx = hm_value_for(hm, key);
        }

        if(_ifh1[i].num_ipv4s > 0) {
            memcpy(merged_ifhs[idx].ipv4s + merged_ifhs[idx].num_ipv4s,
            _ifh1[i].ipv4s, sizeof(ipv4_range) * _ifh1[i].num_ipv4s);
            merged_ifhs[idx].num_ipv4s += _ifh1[i].num_ipv4s;
        }

        if(_ifh1[i].num_ipv6s > 0) {
            memcpy(merged_ifhs[idx].ipv6s + merged_ifhs[idx].num_ipv6s,
            _ifh1[i].ipv6s, sizeof(ipv6_range) * _ifh1[i].num_ipv6s);
            merged_ifhs[idx].num_ipv6s += _ifh1[i].num_ipv6s;
        }
    }

    for(size_t i = 0; i < _ifh2_num_ifhs; i++) {
        size_t key = cantor_pairing_hash(_ifh2[i].if_index, _ifh2[i].mode);

        if(!hm_exists(hm, key)) {
            idx = c;
            hm_insert(hm, key, c++);

            merged_ifhs[idx].num_ipv4s = 0;
            merged_ifhs[idx].num_ipv6s = 0;

            size_t num_ipv4s = hm_value_for(ips_hm_ipv4, key);
            if(num_ipv4s > 0) {
                merged_ifhs[idx].ipv4s = malloc(sizeof(ipv4_range)
                    * num_ipv4s);
                if(!merged_ifhs[idx].ipv4s) {
                    return NULL;
                }
            }

            size_t num_ipv6s = hm_value_for(ips_hm_ipv6, key);
            if(num_ipv6s > 0) {
                merged_ifhs[idx].ipv6s = malloc(sizeof(ipv6_range)
                    * num_ipv6s);
                if(!merged_ifhs[idx].ipv6s) {
                    return NULL;
                }
            }

            merged_ifhs[idx].if_index = _ifh2[i].if_index;
            merged_ifhs[idx].mode = _ifh2[i].mode;
        } else {
            idx = hm_value_for(hm, key);
        }

        if(_ifh2[i].num_ipv4s > 0) {
            memcpy(merged_ifhs[idx].ipv4s + merged_ifhs[idx].num_ipv4s,
            _ifh2[i].ipv4s, sizeof(ipv4_range) * _ifh2[i].num_ipv4s);
            merged_ifhs[idx].num_ipv4s += _ifh2[i].num_ipv4s;
        }

        if(_ifh2[i].num_ipv6s > 0) {
            memcpy(merged_ifhs[idx].ipv6s + merged_ifhs[idx].num_ipv6s,
            _ifh2[i].ipv6s, sizeof(ipv6_range) * _ifh2[i].num_ipv6s);
            merged_ifhs[idx].num_ipv6s += _ifh2[i].num_ipv6s;
        }
    }

    hm_free(hm);
    hm_free(ips_hm_ipv4);
    hm_free(ips_hm_ipv6);
    *_out_num_ifhs = c;
    return merged_ifhs;
}

int _compare_uint32(const void* a, const void* b) {
    uint32_t val_a = *(const uint32_t*)a;
    uint32_t val_b = *(const uint32_t*)b;
    if (val_a < val_b) return -1;
    else if (val_a > val_b) return 1;
    else return 0;
}

uint32_t* ifh_get_uniq_idx(if_h* _ifhs, size_t _num_fhs, size_t* _num_idxs) {
    uint32_t* temp = malloc(_num_fhs * sizeof(uint32_t));
    if (temp == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < _num_fhs; ++i) {
        temp[i] = _ifhs[i].if_index;
    }

    qsort(temp, _num_fhs, sizeof(uint32_t), _compare_uint32);

    uint32_t* unique = malloc(_num_fhs * sizeof(uint32_t));
    if (unique == NULL) {
        free(temp);
        return NULL;
    }

    size_t unique_count = 0;
    unique[unique_count++] = temp[0];

    for (size_t i = 1; i < _num_fhs; ++i) {
        if (temp[i] != temp[i - 1]) {
            unique[unique_count++] = temp[i];
        }
    }

    uint32_t* uni = realloc(unique, unique_count * sizeof(uint32_t));
    if (uni == NULL) {
        uni = unique;
    }

    free(temp);
    *_num_idxs = unique_count;
    return uni;
}

int ifh_root_add_cli_db(if_h** p_ifhs, size_t _num_ifhs,
    config_psd_section* _sections, size_t _num_section) {
    size_t num_lines;
    cli_db_line* cdb = cdb_read(CLI_DB_PATH, &num_lines);
    if(!cdb) {
        return CDB_ERROR_FAILED_TO_READ;
    }

    size_t num_cdb_add_rngs;
    if_h* cdb_add_rngs = cdb_2_ifh(cdb, num_lines, &num_cdb_add_rngs,
        _sections, _num_section, CDB_RT_ONLY_ADD);
    if(!cdb_add_rngs) {
        return CDB_ERROR_OTHER;
    }

    size_t num_merged_rngs;
    if_h* merged_rngs = ifh_merge(*p_ifhs, _num_ifhs,
        cdb_add_rngs, num_cdb_add_rngs, &num_merged_rngs);
    if(merged_rngs == NULL) {
        return CDB_ERROR_OTHER;
    }

    ifh_make_valid(merged_rngs, num_merged_rngs);
    ifh_free(*p_ifhs, _num_ifhs);
    ifh_free(cdb_add_rngs, num_cdb_add_rngs);
    *p_ifhs = merged_rngs;

    size_t num_cdb_rm_rngs;
    if_h* cdb_rm_rngs = cdb_2_ifh(cdb, num_lines, &num_cdb_rm_rngs,
        _sections, _num_section, CDB_RT_ONLY_RM);
    if(!cdb_rm_rngs) {
        return CDB_ERROR_OTHER;
    }

    for(size_t i = 0; i < num_cdb_rm_rngs; i++) {
        for(size_t y = 0; y < _num_ifhs; y++) {
            if(cdb_rm_rngs[i].if_index == (*p_ifhs)[y].if_index
            && cdb_rm_rngs[i].mode == (*p_ifhs)[y].mode) {
                if((*p_ifhs)[i].num_ipv4s > 0) {
                    ifh_rm_rngs_ipv4(&(*p_ifhs)[i].ipv4s,
                        (*p_ifhs)[i].num_ipv4s, cdb_rm_rngs[y].ipv4s,
                        cdb_rm_rngs[y].num_ipv4s, &(*p_ifhs)[i].num_ipv4s);
                }
                if((*p_ifhs)[i].num_ipv6s > 0) {
                    ifh_rm_rngs_ipv6(&(*p_ifhs)[i].ipv6s,
                        (*p_ifhs)[i].num_ipv6s, cdb_rm_rngs[y].ipv6s,
                        cdb_rm_rngs[y].num_ipv6s, &(*p_ifhs)[i].num_ipv6s);
                }
            }
        }
    }

    free(cdb);
    ifh_free(cdb_rm_rngs, num_cdb_rm_rngs);

    return CDB_SUCCESS;
}