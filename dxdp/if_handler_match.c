#include "dxdp/if_handler.h"
#include "dxdp/mem_util.h"
#include "dxdp/string_util.h"
#include "dxdp/log.h"
#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

if_index* _ifh_match_by_name(const if_index* _if_indexes,
size_t _num_if_indexes, const char* _name) {
    for (size_t i = 0; i < _num_if_indexes; i++) {
        if (strcmp(_if_indexes[i].name, _name) == 0) {
            return (if_index*) &_if_indexes[i];
        }
    }
    return NULL;
}

if_index* _ifh_match_by_index(const if_index* _if_indexes,
size_t _num_if_indexes, uint32_t _index) {
    for (size_t i = 0; i < _num_if_indexes; i++) {
        if (_if_indexes[i].index == _index) {
            return (if_index*) &_if_indexes[i];
        }
    }
    return NULL;
}

if_index* _ifh_match_by_mac(const if_index* _if_indexes,
size_t _num_if_indexes, const uint8_t _mac[6]) {
    for (size_t i = 0; i < _num_if_indexes; i++) {
        for (size_t j = 0; j < _if_indexes[i].num_interfaces; j++) {
            if (memcmp(_if_indexes[i].interfaces[j].mac, _mac, 6) == 0) {
                return (if_index*) &_if_indexes[i];
            }
        }
    }
    return NULL;
}

if_index* _ifh_match_by_ipv4_addr(const if_index* _if_indexes,
size_t _num_if_indexes, uint32_t _ipv4) {
    for(size_t i = 0; i < _num_if_indexes; i++) {
        for (size_t j = 0; j < _if_indexes[i].num_interfaces; j++) {
            if (_if_indexes[i].interfaces[j].ipv4 == _ipv4) {
                return (if_index*) &_if_indexes[i];
            }
        }
    }

    return NULL;
}

if_index* _ifh_match_by_ipv6_addr(const if_index* _if_indexes,
size_t _num_if_indexes, const uint8_t _ipv6[16]) {
    for(size_t i = 0; i < _num_if_indexes; i++) {
        for (size_t j = 0; j < _if_indexes[i].num_interfaces; j++) {
            if (memcmp(
                (void*) &_if_indexes[i].interfaces[j].ipv6,
                (void*) &_ipv6,
                sizeof(&_if_indexes[i].interfaces[j].ipv6))
            == 0) {
                return (if_index*) &_if_indexes[i];
            }
        }
    }

    return NULL;
}

if_index** _ifh_match_by_ipv4_subnet(const if_index* _if_indexes,
size_t _num_if_indexes, uint32_t _subnet, size_t* _num_found_indexes) {
    size_t c = 0;
    if_index** p_matches = malloc(sizeof(if_index*) * MAX_MATCHES);
    if(!p_matches) {
        return NULL;
    }

    for (size_t i = 0; i < _num_if_indexes; i++) {
        for (size_t j = 0; j < _if_indexes[i].num_interfaces; j++) {
            if (memcmp(&_if_indexes[i].interfaces[j].ipv4, &_subnet,
                sizeof(uint32_t)) == 0) {
                p_matches[c++] = (if_index*) &_if_indexes[i];
                break;
            }
        }
    }

    *_num_found_indexes = c;

    if(c == 0) {
        free(p_matches);
        return NULL;
    } else {
        p_matches = realloc(p_matches, sizeof(if_index*) * c);
    }

    return p_matches;
}

if_index** _ifh_match_by_ipv6_prefix(const if_index* _if_indexes,
size_t _num_if_indexes, uint64_t _ipv6_prefix, uint32_t _prefix_length_bytes,
size_t* _num_found_indexes) {
    size_t c = 0;
    if_index** p_matches = malloc(sizeof(if_index*) * MAX_MATCHES);
    if(!p_matches) {
        return NULL;
    }

    for (size_t i = 0; i < _num_if_indexes; i++) {
        for (size_t j = 0; j < _if_indexes[i].num_interfaces; j++) {
            if (memcmp_bits(&_if_indexes[i].interfaces[j].ipv6,
                &_ipv6_prefix,
                _prefix_length_bytes * 8)
                == 0) {
                p_matches[c++] = (if_index*) &_if_indexes[i];
                break;
            }
        }
    }

    *_num_found_indexes = c;

    if(c == 0) {
        free(p_matches);
        return NULL;
    } else {
        p_matches = realloc(p_matches, sizeof(if_index*) * c);
    }

    return p_matches;
}

if_index** ifh_match_if(const if_index* _if_indexes,
size_t _num_if_indexes, config_section* p_section,
size_t* _num_indexes_matches) {
    if_index** matches = malloc((sizeof(if_index*) * _num_if_indexes) + 1000);

    *_num_indexes_matches = 0;
    size_t num_matches = 0;
    char valbuff[CONFIG_MAX_LINE_LEN];

    if (config_get_val(p_section, "by_name", valbuff)) {
        const char* by_name = valbuff;
        size_t num_names;
        const char** by_names = su_str_split(by_name, ',', &num_names);

        for(size_t y = 0; y < num_names; y++) {
            if_index* found = _ifh_match_by_name(_if_indexes, _num_if_indexes,
            by_names[y]);

            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: Interface with name '%s' was"
                " not found.\n", by_names[y]);
                su_free_split(by_names, num_names);
                return NULL;
            }

            matches[num_matches++] = found;
            if(num_matches >= MAX_MATCHES) {
                su_free_split(by_names, num_names);
                goto end;
            }
        }

        su_free_split(by_names, num_names);
    }

    if(config_get_val(p_section, "by_index", valbuff)) {
        const char* by_index = valbuff;
        size_t num_indexes;
        const char** by_indexes = su_str_split(by_index, ',', &num_indexes);

        for(size_t y = 0; y < num_indexes; y++) {
            if(strlen(by_indexes[y]) > 7) {
                log_write(LL_FATAL, "CONFIG ERROR: Config value '%s' is "
                    "too long. \n", by_indexes[y]);
                su_free_split(by_indexes, num_indexes);
                return NULL;
            }

            bool valid = true;
            for(size_t z = 0; z < strlen(by_indexes[y]); z++) {
                if(by_indexes[y][z] < '0' || by_indexes[y][z] > '9') {
                    valid = false;
                }
            }

            if(!valid) {
                log_write(LL_FATAL, "CONFIG ERROR: '%s' is not a valid "
                "interface index.\n", by_indexes[y]);
                su_free_split(by_indexes, num_indexes);
                return NULL;
            }

            uint32_t value = strtoul(by_indexes[y], NULL, 10);
            if_index* found = _ifh_match_by_index(_if_indexes,
                _num_if_indexes, value);
            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: Interface with index '%s'"
                    "was not found.\n", by_indexes[y]);
                su_free_split(by_indexes, num_indexes);
                return NULL;
            }

            matches[num_matches++] = found;
            if(num_matches >= MAX_MATCHES) {
                su_free_split(by_indexes, num_indexes);
                goto end;
            }
        }

        su_free_split(by_indexes, num_indexes);
    }


    if(config_get_val(p_section, "by_mac", valbuff)) {
        const char* by_mac = valbuff;
        size_t num_macs;
        const char** by_macs = su_str_split(by_mac, ',', &num_macs);

        for(size_t y = 0; y < num_macs; y++) {
            size_t len = strlen(by_macs[y]);

            if(len > MAC_ADDRSTRLEN) {
                log_write(LL_FATAL, "CONFIG ERROR: MAC address '%s' is too "
                  "long.\n", by_macs[y]);
                su_free_split(by_macs, num_macs);
                return NULL;
            }

            uint8_t mac[6];
            if (sscanf(by_macs[y], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&mac[0],
            &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
                log_write(LL_FATAL, "CONFIG ERROR: Failed to parse MAC "
                  "address '%s'.\n", by_macs[y]);
                su_free_split(by_macs, num_macs);
                return NULL;
            }

            if_index* found = _ifh_match_by_mac(_if_indexes, _num_if_indexes,
                mac);
            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: Interface with MAC address "
                  "'%s' was not found.\n", by_macs[y]);
                su_free_split(by_macs, num_macs);
                return NULL;
            }

            matches[num_matches++] = found;
            if(num_matches >= MAX_MATCHES) {
                su_free_split(by_macs, num_macs);
                goto end;
            }
        }

        su_free_split(by_macs, num_macs);
    }

    if(config_get_val(p_section, "by_ipv4", valbuff)) {
        const char* by_ipv4 = valbuff;
        size_t num_ipv4s;
        const char** by_ipv4s = su_str_split(by_ipv4, ',', &num_ipv4s);

        for(size_t y = 0; y < num_ipv4s; y++) {
            size_t len = strlen(by_ipv4s[y]);

            if(len > INET_ADDRSTRLEN) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 value '%s' is too "
                    "long. \n", by_ipv4s[y]);
                su_free_split(by_ipv4s, num_ipv4s);
                return NULL;
            }

            uint32_t ip = 0;
            if (inet_pton(AF_INET, by_ipv4s[y], &ip) <= 0) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 value '%s' is "
                    "incorrect.\n", by_ipv4s[y]);
                su_free_split(by_ipv4s, num_ipv4s);
                return NULL;
            }

            ip = ntohl(ip);

            if_index* found = _ifh_match_by_ipv4_addr(_if_indexes,
                _num_if_indexes, ip);
            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 '%s' from config file "
                    "not found.\n", by_ipv4s[y]);
                su_free_split(by_ipv4s, num_ipv4s);
                return NULL;
            }

            matches[num_matches++] = found;
            if(num_matches >= MAX_MATCHES) {
                su_free_split(by_ipv4s, num_ipv4s);
                goto end;
            }
        }

        su_free_split(by_ipv4s, num_ipv4s);
    }

    if(config_get_val(p_section, "by_ipv6", valbuff)) {
        const char* by_ipv6 = valbuff;
        size_t num_ipv6s;
        const char** by_ipv6s = su_str_split(by_ipv6, ',', &num_ipv6s);

        for(size_t y = 0; y < num_ipv6s; y++) {
            size_t len = strlen(by_ipv6s[y]);
            uint8_t ipv6[16];

            if(len > INET6_ADDRSTRLEN) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV6 address '%s' is too "
                    "long. \n", by_ipv6s[y]);
                su_free_split(by_ipv6s, num_ipv6s);
                return NULL;
            }

            if (inet_pton(AF_INET6, by_ipv6s[y], ipv6) != 1) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV6 address '%s' is in the "
                    "incorrect format.\n", by_ipv6s[y]);
                su_free_split(by_ipv6s, num_ipv6s);
                return NULL;
            }

             if_index* found = _ifh_match_by_ipv6_addr(_if_indexes,
                _num_if_indexes, ipv6);
            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV6 '%s' from config file "
                    "not found.\n", by_ipv6s[y]);
                su_free_split(by_ipv6s, num_ipv6s);
                return NULL;
            }

            matches[num_matches++] = found;
            if(num_matches >= MAX_MATCHES) {
                 su_free_split(by_ipv6s, num_ipv6s);
                 goto end;
            }
        }

        su_free_split(by_ipv6s, num_ipv6s);
    }

    if(config_get_val(p_section, "by_ipv4s_subnet", valbuff)) {
        const char* by_ipv4s_subnet = valbuff;
        size_t num_subnets;
        const char** by_ipv4s_subnets = su_str_split(by_ipv4s_subnet,
            ',', &num_subnets);

        for(size_t y = 0; y < num_subnets; y++) {
            size_t len = strlen(by_ipv4s_subnets[y]);

            if(len > INET_ADDRSTRLEN) {
                log_write(LL_FATAL,  "CONFIG ERROR: IPV4 value '%s' is "
                    "too long. \n", by_ipv4s_subnets[y]);
                su_free_split(by_ipv4s_subnets, num_subnets);
                return NULL;
            }

            uint32_t ip = 0;
            if (inet_pton(AF_INET, by_ipv4s_subnets[y], &ip) <= 0) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 value '%s' "
                    "is incorrect.\n", by_ipv4s_subnet);
                su_free_split(by_ipv4s_subnets, num_subnets);
                return NULL;
            }

            size_t num_found_indexes;
            if_index** found = _ifh_match_by_ipv4_subnet(_if_indexes,
                _num_if_indexes, ip, &num_found_indexes);
            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 value '%s' "
                    "is incorrect.\n", by_ipv4s_subnet);
                su_free_split(by_ipv4s_subnets, num_subnets);
                return NULL;
            }

            for(size_t z = 0; z < num_found_indexes; z++) {
                matches[num_matches++] = found[z];
                if(num_matches >= MAX_MATCHES) {
                    su_free_split(by_ipv4s_subnets, num_subnets);
                    goto end;
                }
            }

            free(found);
        }

        su_free_split(by_ipv4s_subnets, num_subnets);
    }

    if(config_get_val(p_section, "by_ipv6s_prefix", valbuff)) {
        const char* by_ipv6s_prefix = valbuff;
        size_t num_prefixes;
        const char** by_ipv6s_prefixes = su_str_split(by_ipv6s_prefix,
            ',', &num_prefixes);

        for(size_t y = 0; y < num_prefixes; y++) {
            size_t len = strlen(by_ipv6s_prefixes[y]);
            if(len > INET6_ADDRSTRLEN) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 prefix '%s' is too "
                    "long. \n", by_ipv6s_prefixes[y]);
                su_free_split(by_ipv6s_prefixes, num_prefixes);
                return NULL;
            }

            uint64_t prefix;
            if (inet_pton(AF_INET6, by_ipv6s_prefixes[y], &prefix) <= 0) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV4 value '%s' is "
                    "incorrect.\n", by_ipv6s_prefixes[y]);
                su_free_split(by_ipv6s_prefixes, num_prefixes);
                return NULL;
            }

            size_t num_found_indexes;
            if_index** found = _ifh_match_by_ipv6_prefix(_if_indexes,
                _num_if_indexes, prefix, len, &num_found_indexes);
            if(!found) {
                log_write(LL_FATAL, "CONFIG ERROR: IPV6 prefix '%s' "
                    "is too long. \n", by_ipv6s_prefixes[y]);
                su_free_split(by_ipv6s_prefixes, num_prefixes);
                return NULL;
            }

            for(size_t z = 0; z < num_found_indexes; z++) {
                matches[num_matches++] = found[z];
                if(num_matches >= MAX_MATCHES) {
                    su_free_split(by_ipv6s_prefixes, num_prefixes);
                    goto end;
                }
            }

            free((void*) found);
        }
        su_free_split(by_ipv6s_prefixes, num_prefixes);
    }

    end:
    if_index** tmp = malloc(sizeof(if_index*) * num_matches);
    size_t t = 0;

    for(size_t i = 0; i < num_matches; i++) {
        tmp[i] = NULL;
    }

    for(size_t i = 0; i < num_matches; i++) {
        bool found = false;

        for(size_t y = 0; y < num_matches; y++) {
            if(matches[i] == tmp[y]) {
                found = true;
                break;
            }
        }

        if(!found) {
            tmp[t++] = matches[i];
        }
    }
    free(matches);

    *_num_indexes_matches = t;
    return tmp;
}

bool ifh_if_index_2_mac(uint32_t _if_index, uint8_t* _dest,
const if_index* _if_indexes, size_t _num_if_indexes) {
    for(size_t i = 0; i < _num_if_indexes; i++) {
        if(_if_indexes[i].index == _if_index) {
            for(size_t y = 0; y < _if_indexes[i].num_interfaces; y++) {
                bool isZeroMac = true;

                for (int z = 0; z < 6; z++) {
                    if (_if_indexes[i].interfaces[y].mac[z] != 0) {
                        isZeroMac = false;
                        break;
                    }
                }

                if(!isZeroMac) {
                    memcpy(_dest, _if_indexes[i].interfaces[y].mac,
                        sizeof(_if_indexes[i].interfaces[y].mac));
                    return true;
                }
            }
        }
    }

    return false;
}

bool ifh_mac_2_if_index(uint8_t mac[6], uint32_t* _dest,
const if_index* _if_indexes, size_t _num_if_indexes) {
    for(size_t i = 0; i < _num_if_indexes; i++) {
        for(size_t y = 0; y < _if_indexes[i].num_interfaces; y++) {
            if(memcmp(mac, _if_indexes[i].interfaces[y].mac,
            sizeof(_if_indexes[i].interfaces[y].mac)) == 0) {
                *_dest = _if_indexes[i].index;
                return true;
            }
        }
    }

    return false;
}

bool ifh_i_2_name(uint32_t _index, const if_index* _if_indexes,
size_t _num_if_indexes, char* _buff_out) {
    for (size_t i = 0; i < _num_if_indexes; ++i) {
        if (_if_indexes[i].index == _index) {
            strcpy(_buff_out, _if_indexes[i].name);
            _buff_out[IFNAMSIZ - 1] = '\0';
            return true;
        }
    }

    return false;
}