#include "dxdp/ipc_action.h"
#include "dxdp/config_parser.h"
#include "dxdp/sysutil.h"
#include "dxdp/db.h"
#include "dxdp/string_util.h"
#include "dxdp/bpf_handler.h"
#include "dxdp/proc_comp.h"
#include "dxdp/if_handler.h"
#include "dxdp/cli_db.h"
#include "dxdp/log.h"
#include "dxdp/options.h"
#include "dxdp/mem_util.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

ipc_message ip_task_file(ip_type type, ip_action action,
const char* _section_name, const char* _path, ldata* data) {
    db_s* db;
    db = db_init();
    config_psd_section sec;
    if(type == IP_TYPE_IPV4) {
        sec.d_type = SECTION_DB_TYPE_IPV4;
    } else {
        sec.d_type = SECTION_DB_TYPE_IPV6;
    }
    sec.s_type = SECTION_TYPE_RANGE;
    sec.mode = SECTION_MODE_WHITELIST;
    strcpy(sec.ipv4_db_path, _path);

    if(!db_read(db, &sec)) {
        return (ipc_message) IPC_MESSAGE_ERROR;
    }

    char** ips;

    if(type == IP_TYPE_IPV4) {
        ips = malloc(sizeof(char*) * (db->num_ipv4 * 2));
    } else {
        ips = malloc(sizeof(char*) * (db->num_ipv6 * 2));
    }

    if(!ips) {
        return (ipc_message) IPC_MESSAGE_ERROR;
    }

    char ipbuff[INET6_ADDRSTRLEN];

    size_t c = 0;

    if(type == IP_TYPE_IPV4) {
        for(size_t i = 0; i < db->num_ipv4; i++) {
            ips[c] = malloc(sizeof(char) * INET6_ADDRSTRLEN);
            if(!ips[c]) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            uint32_t low = db->ipv4s[i].low;
            if (inet_ntop(AF_INET, &low, ipbuff, sizeof(low)) == NULL) {
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            strcpy(ips[c], ipbuff);
            c++;

            ips[c] = malloc(sizeof(char) * INET6_ADDRSTRLEN);
            if(!ips[c]) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            uint32_t high = db->ipv4s[i].high;
            if (inet_ntop(AF_INET, &high, ipbuff, sizeof(low)) == NULL) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            strcpy(ips[c], ipbuff);
            c++;
        }
    } else {
        for(size_t i = 0; i < db->num_ipv6; i++) {
            ips[c] = malloc(sizeof(char) * INET6_ADDRSTRLEN);
            if(!ips[c]) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            uint8_t* low = db->ipv6s[i].low;
            if (inet_ntop(AF_INET6, &low, ipbuff, sizeof(low)) == NULL) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            strcpy(ips[c], ipbuff);
            c++;

            ips[c] = malloc(sizeof(char) * INET6_ADDRSTRLEN);
            if(!ips[c]) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            uint8_t* high = db->ipv6s[i].high;
            if (inet_ntop(AF_INET6, &high, ipbuff, sizeof(low)) == NULL) {
                for(size_t y = 0; y < c; y++) {
                    free(ips[y]);
                }
                free(ips);
                return (ipc_message) IPC_MESSAGE_ERROR;
            }
            strcpy(ips[c], ipbuff);
            c++;
        }
    }

    ipc_message res = ip_task_mem(type, action, _section_name,
        (const char (*)[5000000]) ips, c, data);
    for(size_t i = 0; i < c; i++) {
        free(ips[i]);
    }
    free(ips);
    return res;
}

ipc_message ip_task_mem(ip_type type, ip_action action, 
const char* _section_name, const char _argv[][MAX_ARG_LENGTH],
size_t _argc, ldata* data) {
    int fatal_error = 0;

    for(size_t i = 2; i < _argc; i+=2) {
        const char* low = _argv[i];
        const char* high = _argv[i + 1];

        if(type == IP_TYPE_IPV4) {
            uint32_t low32;
            if(!inet_pton(AF_INET, low, &low32)) {
                return (ipc_message) IPC_MESSAGE_INVALID_IP;
            }

            uint32_t high32;
            if(!inet_pton(AF_INET, high, &high32)) {
                return (ipc_message) IPC_MESSAGE_INVALID_IP;
            }

            if(low32 > high32) {
                return (ipc_message) IPC_MESSAGE_INVALID_IP;
            }
        } else {
            uint8_t low128[16];
            if(!inet_pton(AF_INET6, low, low128)) {
                return (ipc_message) IPC_MESSAGE_INVALID_IP;
            }

            uint8_t high128[16];
            if(!inet_pton(AF_INET6, high, high128)) {
                return (ipc_message) IPC_MESSAGE_INVALID_IP;
            }

            //TODO: Use memcmp instead of __uint128_t
            __uint128_t low128t = u8_2_128(low128);
            __uint128_t high128t = u8_2_128(high128);
        
            if(low128t > high128t) {
                return (ipc_message) IPC_MESSAGE_INVALID_IP;
            }
        }
        bool t = cdb_rm_range(CLI_DB_PATH, low, high, type == IP_TYPE_IPV4);
        if(!t) {
            printf("cdb_rm_range failed...\n");
            fatal_error = 1;
            goto fatal;
        }

        cli_db_line line;
        if(type == IP_TYPE_IPV4) {
            strcpy(line.type, "4");
        } else {
            strcpy(line.type, "6");
        }

        strcpy(line.section_name, _section_name);
        strcpy(line.low, low);
        strcpy(line.high, high);

        if(action == IPA_ADD) {
            strcpy(line.action, "+");
        } else {
            strcpy(line.action, "n");
        }

        if(!cdb_append(DEFAULT_CDB_PATH, &line)) {
            log_write(LL_ERROR, "CDB append failed.\n");
        }
    }

    config_psd_section* sections = data->psd_sections;
    size_t num_sections = data->num_psd_sections;

    size_t* num_root_ifhs = data->num_root_ifhs;

    if(ifh_root_add_cli_db(data->root_ifh, *num_root_ifhs, sections,
    num_sections) != CDB_SUCCESS) {
        return (ipc_message) IPC_MESSAGE_INVALID_IP;
    }

    if_h* root_ifhs = *(data->root_ifh);

    size_t num_unique_ifs;
    uint32_t* unique_ifs = ifh_get_uniq_idx(root_ifhs, *num_root_ifhs,
        &num_unique_ifs);

    config_psd_section* p_cps = section_name_2_psd(_section_name, sections,
        num_sections);
    if(p_cps == NULL) {
        return (ipc_message) IPC_MESSAGE_SECTION_DOES_NOT_EXIST;
    }
    if_index** p_effected_ifs = p_cps->matched_interfaces;
    size_t num_effected_ifs = p_cps->num_interfaces_matched;

    for(size_t i = 0; i < num_unique_ifs; i++) {
        for(size_t y = 0; y < num_effected_ifs; y++) {
            if(unique_ifs[i] == p_effected_ifs[y]->index) {
                const char* template_str = eebpf_create_template_2(root_ifhs,
                    *num_root_ifhs, unique_ifs[i]);
                if(!template_str) {
                    fatal_error = 1;
                    goto fatal;
                }

                char* out = malloc(sizeof(char) * (MAX_ARG_LENGTH * MAX_ARG_COUNT));
                size_t out_size;
                size_t in_size = strlen(template_str);

                if(!pc_comp(template_str, in_size, out, &out_size)) {
                    fatal_error = 1;
                    goto fatal;
                }

                eebpf_detach_force(p_effected_ifs[y]->index);

                eebpf_load_and_attach_file(out, out_size, true,
                    p_effected_ifs[y]->index);
                free(out);
                free((void*) template_str);
                break;
            }
        }
    }

    fatal:
    if(fatal_error == 1) {
        log_write(LL_FATAL, "Fatal error detected.\n");
        data->srv->enabled = false;
    }

    free(unique_ifs);
    return (ipc_message) IPC_MESSAGE_SUCCESS;
}

ipc_packet* get_status(ldata* data) {
    ipc_packet* packet = ipc_new_packet();
    packet->message = IPC_MESSAGE_ERROR;
    packet->argc = 0;
    if(*data->num_root_ifhs > MAX_ARG_COUNT
    || *data->num_root_ifhs >= UINT32_MAX) {
        free(packet);
        return NULL;
    }

    for(size_t i = 0; i < *data->num_root_ifhs; i++) {
        if_h* root_ifh = *(data->root_ifh);

        status_packet sp;
        sp.if_index = root_ifh[i].if_index;
        sp.mode = root_ifh[i].mode;
        sp.num_ipv4s = root_ifh[i].num_ipv4s;
        sp.num_ipv6s = root_ifh[i].num_ipv6s;
        ifh_i_2_name(sp.if_index, data->if_indexes, data->num_if_indexes,
            sp.if_name);
        memcpy(packet->argv[i], &sp, sizeof(status_packet));
    }

    packet->argc = (uint32_t) *data->num_root_ifhs;
    return packet;
}

