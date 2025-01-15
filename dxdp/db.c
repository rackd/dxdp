#include "dxdp/db.h"
#include "dxdp/iso3166.h"
#include "dxdp/mem_util.h"
#include "dxdp/string_util.h"
#include "dxdp/log.h"
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>

db_s* db_init() {
    db_s* db = malloc(sizeof(db_s));
    db->ipv4s = NULL;
    db->num_ipv4 = 0;
    db->ipv6s = NULL;
    db->num_ipv6 = 0;
    return db;
}

bool db_read(db_s* _db, config_psd_section* _p_section){
    file_type file_type;
    const char* path;
    bool should_loop_back = false;

    switch((_p_section->d_type << 4) | _p_section->s_type) {
        case (SECTION_DB_TYPE_IPV4 << 4) | SECTION_TYPE_GEO:
            file_type = FILE_TYPE_GEO_IPV4_CSV;
            break;
        case (SECTION_DB_TYPE_IPV4 << 4) | SECTION_TYPE_RANGE:
            file_type = FILE_TYPE_IPV4_CSV;
            break;
        case (SECTION_DB_TYPE_IPV6 << 4) | SECTION_TYPE_GEO:
             file_type = FILE_TYPE_GEO_IPV6_CSV;
            break;
        case (SECTION_DB_TYPE_IPV6 << 4) | SECTION_TYPE_RANGE:
            file_type = FILE_TYPE_IPV6_CSV;
            break;
        case (SECTION_DB_TYPE_IPV4_AND_IPV6 << 4) | SECTION_TYPE_GEO:
            file_type = FILE_TYPE_GEO_IPV4_CSV;
            should_loop_back = true;
            break;
        case (SECTION_DB_TYPE_IPV4_AND_IPV6 << 4) | SECTION_TYPE_RANGE:
            file_type = FILE_TYPE_IPV4_CSV;
            should_loop_back = true;
            break;
        default:
            file_type = FILE_TYPE_NONE;
            return false;
    }

    _db->f_type = file_type;

    loop_back:

    if(file_type == FILE_TYPE_GEO_IPV4_CSV || file_type == FILE_TYPE_IPV4_CSV) {
        path = _p_section->ipv4_db_path;
    } else {
        path = _p_section->ipv6_db_path;
    }

    FILE* fp = fopen(path, "r");
    if (!fp) {
        // TODO: Let's help the user...add check to see if DB is default
        log_write(LL_FATAL,
            "ERROR: Failed to open file '%s'. If you are using the default"
            " geolocation database, run 'dxdp-db-update'\n", path);
        return false;
    }

    struct stat s;
    if (stat(path, &s) != 0) {
        log_write(LL_FATAL, "DB ERROR: Failed to stat file '%s'.\n", path);
        fclose(fp);
        return false;
    }

    size_t line_len = (INET_ADDRSTRLEN * 2) + 4;
    size_t file_size = s.st_size;
    size_t approx_num_lines = (file_size / line_len) + 100;

    if(file_type == FILE_TYPE_GEO_IPV4_CSV || file_type == FILE_TYPE_IPV4_CSV) {
        _db->ipv4s = malloc(sizeof(db_ipv4) * approx_num_lines);
        if(!_db->ipv4s) {
            log_write(LL_FATAL, "DB ERROR: Mem alloc.\n");
            fclose(fp);
            return false;
        }
    } else {
        _db->ipv6s = malloc(sizeof(db_ipv4) * approx_num_lines);
        if(!_db->ipv6s) {
            log_write(LL_FATAL, "DB ERROR: Mem alloc.\n");
            fclose(fp);
            return false;
        }
    }

    char line[DB_LINE_BUFFER_LENGTH];
    size_t c = 0;
    while (fgets(line, sizeof(line), fp)) {
        if(line[0] == '\n') {
            continue;
        }

        if(strchr(line, '\n') == NULL) {
            db_free(_db);
            fclose(fp);
            log_write(LL_FATAL, "ERROR: Line %zu: '%s' is in the "
                "incorrect format (too long).\n", c + 1, line);

            log_write(LL_FATAL, "If this is the last line, be sure "
                "there's a blank new line after it.\n");

            return false;
        }

        line[strcspn(line, "\n")] = '\0';

        size_t num_tokens;
        const char** tokens = su_str_split(line, ',', &num_tokens);

        size_t expected_tokens;
        if(file_type == FILE_TYPE_GEO_IPV4_CSV ||
        file_type == FILE_TYPE_GEO_IPV6_CSV) {
            expected_tokens = 3;
        } else {
            expected_tokens = 2;
        }

        if(num_tokens != expected_tokens) {
            su_free_split(tokens, num_tokens);
            db_free(_db);
            fclose(fp);
            log_write(LL_FATAL, "ERROR: Line %zu: '%s' is in the"
                " incorrect format (too long).\n", c + 1, line);
            return NULL;
        }

        const char* cc;
        const char* ip_low;
        const char* ip_high;

        // Only look for CC if section type is of geo.
        if(file_type == FILE_TYPE_GEO_IPV4_CSV ||
            file_type == FILE_TYPE_GEO_IPV6_CSV) {
            cc = tokens[0];
            ip_low = tokens[1];
            ip_high = tokens[2];

            c_code code;
            if (strlen(cc) != 2 || (code = cc_to_idx(cc)) == -1) {
                su_free_split(tokens, num_tokens);
                db_free(_db);
                fclose(fp);
                log_write(LL_ERROR, "ERROR: Line %zu: '%s' contains invalid"
                    " country code.\n", c + 1, line);

                return false;
            }

            // Skip adding this entry if country code is not in config.
            bool found = arr_contains(_p_section->c_codes,
                _p_section->num_c_codes, &code, sizeof(code));

            if(!found) {
                su_free_split(tokens, num_tokens);
                continue;
            }

            if (file_type == FILE_TYPE_GEO_IPV4_CSV) {
                _db->ipv4s[c].cc = code;
            } else {
                _db->ipv6s[c].cc = code;
            }
        } else {
            ip_low = tokens[0];
            ip_high = tokens[1];
        }

        uint32_t ipv4_addr_low;
        uint32_t ipv4_addr_high;
        uint8_t ipv6_addr_low[16];
        uint8_t ipv6_addr_high[16];

        bool is_dec = false;
        if(su_str_numeric(ip_low)) {
            is_dec = true;
        }

        int res;
        if((file_type == FILE_TYPE_GEO_IPV4_CSV
        || file_type == FILE_TYPE_IPV4_CSV)) {
            if(is_dec) {
                if(!(su_str_2_uint32(ip_low, &ipv4_addr_low)
                    && su_str_2_uint32(ip_high, &ipv4_addr_high))) {
                    su_free_split(tokens, num_tokens);
                    db_free(_db);
                    fclose(fp);
                    log_write(LL_FATAL, "DB ERROR: Failed to convert"
                        " string to uint32\n");
                    return false;
                }
            } else {
                res = inet_pton(AF_INET, ip_low, &ipv4_addr_low) +
                    inet_pton(AF_INET, ip_high, &ipv4_addr_high);
            }
        } else {
            if(is_dec) {
                if(!(su_str_2_uint128(ip_low, ipv6_addr_low)
                    && su_str_2_uint128(ip_high, ipv6_addr_high))) {
                    su_free_split(tokens, num_tokens);
                    db_free(_db);
                    fclose(fp);
                    log_write(LL_FATAL, "DB ERROR: Failed to convert"
                        " string to uint128\n");
                    return false;
                }
            } else {
                res = inet_pton(AF_INET6, ip_low, ipv6_addr_low) +
                    inet_pton(AF_INET6, ip_high, ipv6_addr_high);
            }
        }

        if(!is_dec) {
            if (res != 2) {
                su_free_split(tokens, num_tokens);
                db_free(_db);
                fclose(fp);

                log_write(LL_FATAL,
                    "Line %zu: '%s' contains invalid %s address.\n",
                    c + 1, line,
                    (file_type == FILE_TYPE_GEO_IPV4_CSV ||
                        FILE_TYPE_IPV4_CSV) ? "IPV4" : "IPV6");
                return false;
            }
        }

        if (file_type == FILE_TYPE_GEO_IPV4_CSV ||
        file_type == FILE_TYPE_IPV4_CSV) {
            if(!is_dec) {
                ipv4_addr_low = ntohl(ipv4_addr_low);
                ipv4_addr_high = ntohl(ipv4_addr_high);
            }
            _db->ipv4s[c].low = ipv4_addr_low;
            _db->ipv4s[c].high = ipv4_addr_high;
        } else {
            memcpy(_db->ipv6s[c].low, ipv6_addr_low,
                sizeof(_db->ipv6s[c].low));
            memcpy(_db->ipv6s[c].high, ipv6_addr_high,
                sizeof(_db->ipv6s[c].low));
        }

        c++;
        if(c >= approx_num_lines) {
            su_free_split(tokens, num_tokens);
            db_free(_db);
            fclose(fp);
            log_write(LL_FATAL, "ERROR: Developer bug... not enough allocated"
                " memory for database. Please submit an issue.\n");
            return false;
        }

        su_free_split(tokens, num_tokens);
    }

    if (file_type == FILE_TYPE_GEO_IPV4_CSV ||
    file_type == FILE_TYPE_IPV4_CSV) {
        _db->num_ipv4 = c;
        _db->ipv4s = realloc(_db->ipv4s, sizeof(db_ipv4) * c);
    } else {
        _db->num_ipv6 = c;
        _db->ipv6s = realloc(_db->ipv6s, sizeof(db_ipv6) * c);
    }

    if(should_loop_back) {
        if(file_type == FILE_TYPE_GEO_IPV4_CSV) {
            file_type = FILE_TYPE_GEO_IPV6_CSV;
            fclose(fp);
            goto loop_back;
        } else if(file_type == FILE_TYPE_IPV4_CSV) {
            file_type = FILE_TYPE_IPV6_CSV;
            fclose(fp);
            goto loop_back;
        }
    }

    fclose(fp);
    return true;
}

void db_free(db_s* _db) {
    free(_db->ipv4s);
    free(_db->ipv6s);
    free(_db);
}