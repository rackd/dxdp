#include "dxdp/cli_db.h"
#include "dxdp/string_util.h"
#include "dxdp/hash_map.h"
#include "dxdp/cantor_hash.h"
#include "dxdp/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

bool cdb_append(const char* _path, cli_db_line* _line) {
    int fd = open(_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        return false;
    }

    FILE *file = fdopen(fd, "a");
    if (fprintf(file, "%s%s%s%s%s%s%s%s%s\n",
            _line->action,
            CDB_DELIM,
            _line->type,
            CDB_DELIM,
            _line->section_name,
            CDB_DELIM,
            _line->low,
            CDB_DELIM,
            _line->high
    ) < 0) {
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

bool cdb_delete(const char* _path) {
    FILE* file = fopen(_path, "w");
    if (file == NULL) {
        return false;
    }
    if (fclose(file) != 0) {
        return false;
    }

    return true;
}

cli_db_line* cdb_read(const char* _path, size_t* _num_entries) {
    FILE* file = fopen(_path, "r");
    if (!file) {
        return NULL;
    }

    size_t num_entries = 100;
    cli_db_line* entries = malloc(sizeof(cli_db_line) * num_entries);
    if (!entries) {
        fclose(file);
        return NULL;
    }

    char line[DB_LINE_BUFFER_LENGTH];
    size_t i = 0;
    while (fgets(line, sizeof(line), file)) {
        if(line[0] == '\n') {
            continue;
        }

         // TODO: DB corruption..perhaps warn? Delete?
        if(strchr(line, '\n') == NULL) {
            fclose(file);
            return NULL;
        }

        line[strcspn(line, "\n")] = '\0';

        size_t num_tokens;
        const char** tokens = su_str_split(line, '-', &num_tokens);
        if(num_tokens != 5) {
            su_free_split(tokens, num_tokens);
            fclose(file);
            return NULL;
        }

        strncpy(entries[i].action, tokens[0], sizeof(entries[i].action));
        strncpy(entries[i].type, tokens[1], sizeof(entries[i].type));
        strncpy(entries[i].section_name, tokens[2],
            sizeof(entries[i].section_name));
        strncpy(entries[i].low, tokens[3], sizeof(entries[i].low));
        strncpy(entries[i].high, tokens[4], sizeof(entries[i].high));

        i++;

        if(i >= (num_entries - 1)) {
            num_entries*=2;
            cli_db_line* tmp = realloc(entries, sizeof(cli_db_line) *
                num_entries);
            if(!tmp) {
                fclose(file);
                su_free_split(tokens, num_tokens);
                return NULL;
            }
            entries = tmp;
        }

        su_free_split(tokens, num_tokens);
    }

    fclose(file);

    cli_db_line* tmp = realloc(entries, sizeof(cli_db_line) * i);
    if(!tmp) {
        return NULL;
    }

    *_num_entries = i;
    return tmp;
}

if_h* cdb_2_ifh(cli_db_line* _cdb, size_t _num_lines,
size_t* _num_ifhs, config_psd_section* _sections, size_t _num_sections,
cdb_read_type rtype) {
    hash_map* hm = hm_new(HM_INIT_SIZE_DEFAULT);
    hash_map* ips_hm_ipv4 = hm_new(HM_INIT_SIZE_DEFAULT);
    hash_map* ips_hm_ipv6 = hm_new(HM_INIT_SIZE_DEFAULT);

    size_t num_ifhs = 0;
    for(size_t i = 0; i < _num_lines; i++) {
        if(rtype == CDB_RT_ONLY_RM) {
            if (strcmp(_cdb[i].action, "n") != 0) {
                continue;
            }
        } else if (rtype == CDB_RT_ONLY_ADD) {
            if (strcmp(_cdb[i].action, "n") == 0) {
                continue;
            }
        }

        config_psd_section* p_cps = section_name_2_psd(_cdb[i].section_name,
            _sections, _num_sections);
        if(p_cps == NULL) {
            continue;
        }

        for(size_t y = 0; y < p_cps->num_interfaces_matched; y++) {
            size_t key = cantor_pairing_hash(
                p_cps->matched_interfaces[y]->index, p_cps->mode);

            if(!hm_exists(hm, key)) {
                hm_insert(hm, key, 0);
                num_ifhs++;
            }

            if(strcmp(_cdb[i].type, "4") == 0) {
                if(!hm_exists(ips_hm_ipv4, key)) {
                    hm_insert(ips_hm_ipv4, key, 1);
                } else {
                    size_t num_ipv4s_key = hm_value_for(ips_hm_ipv4, key);
                    hm_insert(ips_hm_ipv4, key, num_ipv4s_key + 1);
                }

                if(!hm_exists(ips_hm_ipv6, key)) {
                    hm_insert(ips_hm_ipv6, key, 0);
                }
            } else {
                if(!hm_exists(ips_hm_ipv6, key)) {
                    hm_insert(ips_hm_ipv6, key, 1);
                } else {
                    size_t num_ipv6s_key = hm_value_for(ips_hm_ipv6, key);
                    hm_insert(ips_hm_ipv6, key, num_ipv6s_key + 1);
                }

                if(!hm_exists(ips_hm_ipv4, key)) {
                    hm_insert(ips_hm_ipv4, key, 0);
                }
            }
        }
    }

    hm_free(hm);
    hm = hm_new(HM_INIT_SIZE_DEFAULT);
    if_h* ifhs = malloc(sizeof(if_h) * num_ifhs);
    size_t c = 0;

    for(size_t i = 0; i < _num_lines; i++) {
        if(rtype == CDB_RT_ONLY_RM) {
            if (strcmp(_cdb[i].action, "n") != 0) {
                continue;
            }
        } else if (rtype == CDB_RT_ONLY_ADD) {
            if (strcmp(_cdb[i].action, "n") == 0) {
                continue;
            }
        }

        config_psd_section* p_cps =
            section_name_2_psd(_cdb[i].section_name, _sections, _num_sections);
        if(p_cps == NULL) {
            continue;
        }

        for(size_t y = 0; y <  p_cps->num_interfaces_matched; y++) {
            size_t key = cantor_pairing_hash(
                p_cps->matched_interfaces[y]->index, p_cps->mode);

            size_t idx = 0;

            if(!hm_exists(hm, key)) {
                ifhs[c].num_ipv4s = 0;
                size_t num_ipv4 = hm_value_for(ips_hm_ipv4, key);
                if(num_ipv4 > 0) {
                    ifhs[c].ipv4s = malloc(sizeof(ipv4_range) * num_ipv4);
                    if(!ifhs[c].ipv4s) {
                        return NULL;
                    }
                }

                ifhs[c].num_ipv6s = 0;
                size_t num_ipv6 = hm_value_for(ips_hm_ipv6, key);
                if(num_ipv6 > 0) {
                    ifhs[c].ipv6s = malloc(sizeof(ipv6_range) * num_ipv6);
                    if(!ifhs[c].ipv6s) {
                        return NULL;
                    }
                }

                ifhs[c].mode = p_cps->mode;
                ifhs[c].if_index = p_cps->matched_interfaces[y]->index;
                idx = c;
                hm_insert(hm, key, c++);
                num_ifhs++;
            } else {
                idx = hm_value_for(hm, key);
            }

            char* low = _cdb[i].low;
            char* high = _cdb[i].high;

            if(strcmp(_cdb[i].type, "4") == 0) {
                uint32_t low_i;
                uint32_t high_i;

                int res = inet_pton(AF_INET, low, &low_i) +
                    inet_pton(AF_INET, high, &high_i);

                if(res != 2) {
                    return NULL;
                }

                low_i = ntohl(low_i);
                high_i = ntohl(high_i);

                ifhs[idx].ipv4s[ifhs[idx].num_ipv4s].low = low_i;
                ifhs[idx].ipv4s[ifhs[idx].num_ipv4s++].high = high_i;
            } else {
                uint8_t low_128[16];
                uint8_t high_128[16];

                int res = inet_pton(AF_INET6, low, &low_128) +
                    inet_pton(AF_INET6, high, &high_128);

                if(res != 2) {
                    return NULL;
                }

                memcpy(ifhs[idx].ipv6s[ifhs[idx].num_ipv6s].low, low_128,
                    sizeof(ifhs[idx].ipv6s[ifhs[idx].num_ipv6s].low));


                memcpy(ifhs[idx].ipv6s[ifhs[idx].num_ipv6s++].high, high_128,
                    sizeof(ifhs[idx].ipv6s[ifhs[idx].num_ipv6s].high));
            }
        }
    }

    *_num_ifhs = c;
    hm_free(hm);
    hm_free(ips_hm_ipv4);
    hm_free(ips_hm_ipv6);
    return ifhs;
}

bool ipv4_str_to_uint32(const char* str, uint32_t* out) {
    struct in_addr addr;
    if (inet_pton(AF_INET, str, &addr) != 1) {
        return false;
    }
    *out = ntohl(addr.s_addr);
    return true;
}

void uint32_to_ipv4_str(uint32_t addr, char* str) {
    struct in_addr in;
    in.s_addr = htonl(addr);
    inet_ntop(AF_INET, &in, str, INET6_ADDRSTRLEN);
}

bool ipv6_str_to_uint8(const char* str, uint8_t* out) {
    struct in6_addr addr6;
    if (inet_pton(AF_INET6, str, &addr6) != 1) {
        return false;
    }
    memcpy(out, addr6.s6_addr, 16);
    return true;
}

void uint8_to_ipv6_str(const uint8_t* addr, char* str) {
    struct in6_addr in6;
    memcpy(in6.s6_addr, addr, 16);
    inet_ntop(AF_INET6, &in6, str, INET6_ADDRSTRLEN);
}

int compare_ipv4_c(uint32_t a, uint32_t b) {
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    return 0;
}

int compare_ipv6_c(const uint8_t* a, const uint8_t* b) {
    return memcmp(a, b, 16);
}

bool ipv6_increment(uint8_t* addr) {
    for (int i = 15; i >= 0; i--) {
        if (addr[i] < 0xFF) {
            addr[i]++;
            return true;
        }
        addr[i] = 0;
    }
    return false; // Overflow
}


bool ipv6_decrement(uint8_t* addr) {
    for (int i = 15; i >= 0; i--) {
        if (addr[i] > 0x00) {
            addr[i]--;
            return true;
        }
        addr[i] = 0xFF;
    }
    return false; // Underflow
}

/* _lines is a pointer to an array of cli_db_line
 * restructures array of cli_db_line such that _low to _high is no longer in the
 * array, if present.
 */
bool cdb_rm_range_lines(cli_db_line** _lines, size_t* _num_lines,
const char* _low, const char* _high, bool _is_ipv4) {
    size_t original_num = *_num_lines;
    size_t new_capacity = original_num * 2;
    cli_db_line* temp = malloc(sizeof(cli_db_line) * new_capacity);
    if (!temp) {
        return false;
    }

    if (*_num_lines == 0) {
        return true;
    }
    
    size_t temp_idx = 0;
    uint32_t low_ipv4 = 0, high_ipv4 = 0;
    uint8_t low_ipv6[16], high_ipv6[16];
    if (_is_ipv4) {
        if (!ipv4_str_to_uint32(_low, &low_ipv4)
        || !ipv4_str_to_uint32(_high, &high_ipv4)) {
            free(temp);
            return false;
        }
        if (compare_ipv4_c(low_ipv4, high_ipv4) > 0) {
            free(temp);
            return false;
        }
    } else {
        if (!ipv6_str_to_uint8(_low, low_ipv6)
        || !ipv6_str_to_uint8(_high, high_ipv6)) {
            free(temp);
            return false;
        }
        if (compare_ipv6_c(low_ipv6, high_ipv6) > 0) {
            free(temp);
            return false;
        }
    }

    for (size_t i = 0; i < original_num; i++) {
        cli_db_line current = (*_lines)[i];
        uint32_t current_low_ipv4 = 0, current_high_ipv4 = 0;
        uint8_t current_low_ipv6[16], current_high_ipv6[16];
        bool conversion_success = false;
        if (_is_ipv4) {
            if (ipv4_str_to_uint32(current.low, &current_low_ipv4) &&
                ipv4_str_to_uint32(current.high, &current_high_ipv4)) {
                conversion_success = true;
            }
        } else {
            if (ipv6_str_to_uint8(current.low, current_low_ipv6) &&
                ipv6_str_to_uint8(current.high, current_high_ipv6)) {
                conversion_success = true;
            }
        }
        if (!conversion_success) {
            continue;
        }

        bool overlap = false;
        if (_is_ipv4) {
            if (!(current_high_ipv4 < low_ipv4
            || current_low_ipv4 > high_ipv4)) {
                overlap = true;
            }
        } else {
            if (!(compare_ipv6_c(current_high_ipv6, low_ipv6) < 0
            || compare_ipv6_c(current_low_ipv6, high_ipv6) > 0)) {
                overlap = true;
            }
        }

        if (!overlap) {
            temp[temp_idx++] = current;
            continue;
        }

        if (_is_ipv4) {
            if (current_low_ipv4 < low_ipv4 && current_high_ipv4 > high_ipv4) {
                cli_db_line part1 = current;
                uint32_to_ipv4_str(current_low_ipv4, part1.low);
                uint32_to_ipv4_str(low_ipv4 - 1, part1.high);
                temp[temp_idx++] = part1;
                cli_db_line part2 = current;
                uint32_to_ipv4_str(high_ipv4 + 1, part2.low);
                uint32_to_ipv4_str(current_high_ipv4, part2.high);
                temp[temp_idx++] = part2;
            } else if (current_low_ipv4 < low_ipv4 && current_high_ipv4
            >= low_ipv4 && current_high_ipv4 <= high_ipv4) {
                cli_db_line adjusted = current;
                uint32_to_ipv4_str(current_low_ipv4, adjusted.low);
                uint32_to_ipv4_str(low_ipv4 - 1, adjusted.high);
                temp[temp_idx++] = adjusted;
            } else if (current_low_ipv4 >= low_ipv4 && current_low_ipv4
            <= high_ipv4 && current_high_ipv4 > high_ipv4) {
                cli_db_line adjusted = current;
                uint32_to_ipv4_str(high_ipv4 + 1, adjusted.low);
                uint32_to_ipv4_str(current_high_ipv4, adjusted.high);
                temp[temp_idx++] = adjusted;
            } else if (current_low_ipv4 >= low_ipv4 &&
            current_high_ipv4 <= high_ipv4) {
                continue;
            } else {
                temp[temp_idx++] = current;
            }
        } else {
            bool current_low_less =
                compare_ipv6_c(current_low_ipv6, low_ipv6) < 0;
            bool current_high_greater =
                compare_ipv6_c(current_high_ipv6, high_ipv6) > 0;

            if (current_low_less && current_high_greater) {
                cli_db_line part1 = current;
                uint8_to_ipv6_str(current_low_ipv6, part1.low);

                uint8_t new_high_ipv6[16];
                memcpy(new_high_ipv6, low_ipv6, 16);
                if (!ipv6_decrement(new_high_ipv6)) {
                    memset(part1.high, 0, INET6_ADDRSTRLEN);
                }
                uint8_to_ipv6_str(new_high_ipv6, part1.high);
                temp[temp_idx++] = part1;

                cli_db_line part2 = current;
                uint8_t new_low_ipv6[16];
                memcpy(new_low_ipv6, high_ipv6, 16);
                if (!ipv6_increment(new_low_ipv6)) {
                    memset(part2.low, 0, INET6_ADDRSTRLEN);
                }
                uint8_to_ipv6_str(new_low_ipv6, part2.low);
                uint8_to_ipv6_str(current_high_ipv6, part2.high);
                temp[temp_idx++] = part2;
            } else if (current_low_less &&
            compare_ipv6_c(current_high_ipv6, low_ipv6) >= 0 &&
            compare_ipv6_c(current_high_ipv6, high_ipv6) <= 0) {
                cli_db_line adjusted = current;
                uint8_to_ipv6_str(current_low_ipv6, adjusted.low);

                uint8_t new_high_ipv6[16];
                memcpy(new_high_ipv6, low_ipv6, 16);
                if (!ipv6_decrement(new_high_ipv6)) {
                    memset(new_high_ipv6, 0, 16);
                }
                uint8_to_ipv6_str(new_high_ipv6, adjusted.high);
                temp[temp_idx++] = adjusted;
            } else if (compare_ipv6_c(current_low_ipv6, low_ipv6) >= 0 &&
            compare_ipv6_c(current_low_ipv6, high_ipv6) <= 0 &&
            compare_ipv6_c(current_high_ipv6, high_ipv6) > 0) {
                cli_db_line adjusted = current;
                uint8_t new_low_ipv6[16];
                memcpy(new_low_ipv6, high_ipv6, 16);
                if (!ipv6_increment(new_low_ipv6)) {
                    memset(new_low_ipv6, 0xFF, 16);
                }
                uint8_to_ipv6_str(new_low_ipv6, adjusted.low);
                uint8_to_ipv6_str(current_high_ipv6, adjusted.high);
                temp[temp_idx++] = adjusted;
            } else if (compare_ipv6_c(current_low_ipv6, low_ipv6) >= 0 &&
            compare_ipv6_c(current_high_ipv6, high_ipv6) <= 0) {
                continue;
            }
            else {
                temp[temp_idx++] = current;
            }
        }
    }

    cli_db_line* resized = realloc(*_lines, sizeof(cli_db_line) * temp_idx);
    if (!resized && temp_idx > 0) {
        free(temp);
        return false;
    }

    *_lines = resized;
    for (size_t i = 0; i < temp_idx; i++) {
        (*_lines)[i] = temp[i];
    }

    *_num_lines = temp_idx;

    free(temp);
    return true;
}


bool cdb_rm_range(const char* _path, const char* _low, const char* _high,
bool is_ipv4) {
    size_t num_entries;
    cli_db_line* cdb = cdb_read(_path, &num_entries);
    if(!cdb) {
        if (errno == ENOENT) {
            return true;
        }

        return false;
    }

    size_t c_ipv4 = 0;
    cli_db_line* ipv4s = malloc(sizeof(cli_db_line) * num_entries);
    if(!ipv4s) {
        free(cdb);
        return false;
    }

    size_t c_ipv6 = 0;
    cli_db_line* ipv6s = malloc(sizeof(cli_db_line) * num_entries);
    if(!ipv6s) {
        free(cdb);
        free(ipv4s);
        return false;
    }

    for(size_t i = 0; i < num_entries; i++) {
        if(strcmp(cdb[i].type, "4") == 0) {
            ipv4s[c_ipv4++] = cdb[i];
        }
    }

    for(size_t i = 0; i < num_entries; i++) {
        if(strcmp(cdb[i].type, "6") == 0) {
            ipv6s[c_ipv6++] = cdb[i];
        }
    }

    if(is_ipv4) {
        if(!cdb_rm_range_lines(&ipv4s, &c_ipv4, _low, _high, true)) {
            free(cdb);
            free(ipv4s);
            free(ipv6s);
            return false;
        }
    } else {
        if(!cdb_rm_range_lines(&ipv6s, &c_ipv6, _low, _high, false)) {
            free(cdb);
            free(ipv4s);
            free(ipv6s);
            return false;
        }
    }

    if(!cdb_delete(_path)) {
        free(cdb);
        free(ipv4s);
        free(ipv6s);
        return false;
    }

    for(size_t i = 0; i < c_ipv4; i++) {
        if(!cdb_append(_path, &ipv4s[i])) {
            free(cdb);
            free(ipv4s);
            free(ipv6s);
            return false;
        }
    }

    for(size_t i = 0; i < c_ipv6; i++) {
        if(!cdb_append(_path, &ipv6s[i])) {
            free(cdb);
            free(ipv4s);
            free(ipv6s);
            return false;
        }
    }

    free(cdb);
    free(ipv4s);
    free(ipv6s);
    return true;
}