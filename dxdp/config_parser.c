#include "dxdp/config_parser.h"
#include "dxdp/if_handler.h"
#include "dxdp/string_util.h"
#include "dxdp/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool _config_has_duplicate_entry_keys(config_section* _sections,
    size_t _num_sections) {
    for (size_t i = 0; i < _num_sections; i++) {
        for (size_t j = 0; j < _sections[i].num_entries; j++) {
            for (size_t k = j + 1; k < _sections[i].num_entries; k++) {
                if (strcmp(_sections[i].entries[j].key,
                    _sections[i].entries[k].key) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool _config_has_blank_keys_or_values(config_section* _sections,
    size_t _num_sections) {
    for (size_t i = 0; i < _num_sections; i++) {
        for (size_t j = 0; j < _sections[i].num_entries; j++) {
            if(strcmp(_sections[i].entries[j].key, "") == 0 ||
                strcmp(_sections[i].entries[j].value, "") == 0) {
                return true;
            }
        }
    }

    return false;
}

bool _config_is_valid(config_section* _sections, size_t _num_sections) {
    if(_config_has_duplicate_entry_keys(_sections, _num_sections)) {
        log_write(LL_FATAL, "CONFIG ERROR: A section cannot contain duplicate"
            " keys. Use commas.\n");
        return false;
    }

    if(_config_has_blank_keys_or_values(_sections, _num_sections)) {
        log_write(LL_FATAL, "CONFIG ERROR: A key or value cannot be blank.\n");
        return false;
    }

    return true;
}

bool _config_is_parsed_valid(config_psd_section* _sections,
    size_t _num_sections) {
    bool is_valid = true;

    for(size_t i = 0; i < _num_sections; i++) {
        if(_sections[i].s_type == SECTION_TYPE_NONE) {
            log_write(LL_FATAL, "CONFIG ERROR: Section '%s' must define a "
                "type.\n", _sections[i].section_name);
            is_valid = false;
        }

        if(_sections[i].mode == SECTION_MODE_NONE) {
            log_write(LL_FATAL, "CONFIG ERROR: Section '%s' does not define a"
                " valid mode.\n", _sections[i].section_name);
            is_valid = false;
        }

        if(_sections[i].s_type == SECTION_TYPE_GEO &&
            _sections[i].num_c_codes == 0) {
            log_write(LL_FATAL, "CONFIG ERROR: Section '%s' is of type geo but "
                "contains no country codes.\n", _sections[i].section_name);
            is_valid = false;
        }

        if(_sections[i].d_type == SECTION_DB_TYPE_NONE) {
            log_write(LL_FATAL, "CONFIG ERROR: Section '%s' must contain at "
                "least one database path.\n", _sections[i].section_name);
            is_valid = false;
        }
    }

    return is_valid;
}

bool _config_parse_cc(c_code* codes, const char* _delimed_c_codes,
    size_t* _num_codes) {
    if(!_delimed_c_codes) return NULL;

    size_t num_codes;
    const char** codes_str = su_str_split(_delimed_c_codes, ',', &num_codes);
    if(codes_str == NULL) {
        *_num_codes = 0;
        log_write(LL_FATAL, "Failed to parse country codes.\n");
        return NULL;
    }

    if(num_codes >= NUM_CODES) {
        log_write(LL_FATAL, "Too many country codes...\n");
        return NULL;
    }

    for(size_t i = 0; i < num_codes; i++) {
        c_code code = cc_to_idx(codes_str[i]);
        if(code == -1) {
            *_num_codes = 0;
            log_write(LL_FATAL, "Config contained unknown country code.\n");
            free(codes);
            su_free_split(codes_str, num_codes);
            return NULL;
        }
        codes[i] = code;
    }

    su_free_split(codes_str, num_codes);

    *_num_codes = num_codes;
    return codes;
}

config_psd_section* config_parse(config_section* _sections,
    size_t _num_sections, const if_index* _if_indexes, size_t _num_if_indexes) {
    if(!_config_is_valid(_sections, _num_sections)) {
        return NULL;
    }

    config_psd_section* psd_sections =
        malloc(sizeof(config_psd_section) * _num_sections);
    if(!psd_sections) {
        return NULL;
    }

    char val_buff[CONFIG_MAX_LINE_LEN];
    char val_buff2[CONFIG_MAX_LINE_LEN];

    for(size_t i = 0; i < _num_sections; i++) {
        strcpy(psd_sections[i].section_name, _sections[i].section_name);

        if_index** matches = ifh_match_if(_if_indexes,
            _num_if_indexes, &_sections[i],
            &psd_sections[i].num_interfaces_matched);

        if(!matches) {
            return NULL;
        }
        psd_sections[i].matched_interfaces = matches;

        psd_sections[i].s_type = SECTION_TYPE_NONE;
        if(config_get_val(&_sections[i], "type", val_buff)) {
            char* type = val_buff;
            su_to_lower(type);
            if(strcmp(type, "geo") == 0) {
                psd_sections[i].s_type = SECTION_TYPE_GEO;
            } else if (strcmp(type, "range") == 0) {
                psd_sections[i].s_type = SECTION_TYPE_RANGE;
            }
        }

        psd_sections[i].mode = SECTION_MODE_NONE;
        if(config_get_val(&_sections[i], "mode", val_buff)) {
            char* mode = val_buff;
            su_to_lower(mode);
            if(strcmp(mode, "whitelist") == 0) {
                psd_sections[i].mode = SECTION_MODE_WHITELIST;
            } else if (strcmp(mode, "blacklist") == 0) {
                psd_sections[i].mode = SECTION_MODE_BLACKLIST;
            }
        }

        psd_sections[i].d_type = SECTION_DB_TYPE_NONE;
        bool has_ipv4 = config_get_val(&_sections[i], "db_ipv4_paths",
            val_buff);
        bool has_ipv6 = config_get_val(&_sections[i], "db_ipv6_paths",
            val_buff2);
        const char* ipv4_db_path = val_buff;
        const char* ipv6_db_path = val_buff2;
        if(has_ipv4 && has_ipv6) {
            strcpy(psd_sections[i].ipv4_db_path, ipv4_db_path);
            strcpy(psd_sections[i].ipv6_db_path, ipv6_db_path);
            psd_sections[i].d_type = SECTION_DB_TYPE_IPV4_AND_IPV6;
        } else if(has_ipv4) {
             psd_sections[i].d_type = SECTION_DB_TYPE_IPV4;
            strcpy(psd_sections[i].ipv4_db_path, ipv4_db_path);
        } else if (has_ipv6) {
            psd_sections[i].d_type = SECTION_DB_TYPE_IPV6;
            strcpy(psd_sections[i].ipv6_db_path, ipv6_db_path);
        }

        psd_sections[i].num_c_codes = 0;
        if(config_get_val(&_sections[i], "cc", val_buff)) {
            const char* cc = val_buff;
            _config_parse_cc(psd_sections->c_codes, cc,
                &psd_sections[i].num_c_codes);
        }

        psd_sections[i].enabled = SECTION_ENABLED_NONE;
        if(config_get_val(&_sections[i], "enabled", val_buff)) {
            char* enabled_str = val_buff;
            su_to_lower(enabled_str);
            if(strcmp(enabled_str, "false") == 0) {
                psd_sections[i].enabled = SECTION_ENABLED_FALSE;
            } else if(strcmp(enabled_str, "true") == 0) {
                psd_sections[i].enabled = SECTION_ENABLED_TRUE;
            }
        }
    }

    if(!_config_is_parsed_valid(psd_sections, _num_sections)) {
        free(psd_sections);
        return NULL;
    }

    return psd_sections;
}


void config_parse_free(config_psd_section* _sections, size_t _num_sections) {
    for(size_t i = 0; i < _num_sections; i++) {
        free((void*)  _sections[i].matched_interfaces);
    }

    free((void*) _sections);
}

config_psd_section* section_name_2_psd(const char* _section_name,
    config_psd_section* _sections, size_t _num_sections) {
    for(size_t i = 0; i < _num_sections; i++) {
        if(strcmp(_sections[i].section_name, _section_name) == 0) {
            return &_sections[i];
            break;
        }
    }

    return NULL;
}