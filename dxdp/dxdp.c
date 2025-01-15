#include "dxdp/iso3166.h"
#include "dxdp/interface.h"
#include "dxdp/config_parser.h"
#include "dxdp/itree_uint32.h"
#include "dxdp/db.h"
#include "dxdp/sysutil.h"
#include "dxdp/if_handler.h"
#include "dxdp/mem_util.h"
#include "dxdp/bpf_handler.h"
#include "dxdp/string_util.h"
#include "dxdp/ipc_action.h"
#include "dxdp/proc.h"
#include "dxdp/proc_comp.h"
#include "dxdp/log.h"
#include "dxdp-ipc/ipc.h"
#include "dxdp/itree_uint128.h"
#include "dxdp/cli_db.h"
#include <dxdp/proc_comp.h>
#include "dxdp/bpf_handler.h"
#include "dxdp/options.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

int dxdp(int argc, char* argv[]) {
    if(!is_root()) {
        fprintf(stderr, "This program must be run as root.\n");
        log_close();
        return 1;
    }

    if(sys_is_already_running()) {
        fprintf(stderr, "dXDP already running.\n");
        fprintf(stderr, "If this is an error delete: %s\n", DEFAULT_PID_PATH);
        log_close();
        return 1;
    }

    char* config_path = CONFIG_PATH;
    if(argc != 1) {
        if(argc == 3 && strcmp(argv[0], "-d") == 0) {
            config_path = argv[1];
        } else {
            fprintf(stdout, "Help: dxdp -d /path/to/config\n");
            return 1;
        }
    }

    if (!log_init()) {
        fprintf(stderr, "Logger initialization failed\n");
        return 1;
    }
    
    set_options();

    int flags = LOG_OUTPUT_STD;

    if(HAS_LOG_FILE) {
        flags = flags | LOG_OUTPUT_FILE;
        log_set_path(LOG_FILE_PATH);
        log_write(LL_INFO, "Using log file: %s.\n", LOG_FILE_PATH);
    }

    log_set_flags(flags);

    log_write(LL_INFO, "dXDP starting.\n");

    log_write(LL_INFO, "Reading config file: %s.\n", config_path);
    size_t num_sections;
    config_section* sections = config_read(config_path, &num_sections);
    if(!sections) {
        return 1;
    }

    log_write(LL_INFO, "Reading interfaces.\n");
    size_t num_if_indexes;
    const if_index* if_indexes = if_get_interfaces(&num_if_indexes);
    if(!if_indexes) {
        config_free(sections);
        return 1;
    };


    log_write(LL_INFO, "Parsing config.\n");
    cc_init();
    config_psd_section* psd_sections = config_parse(sections, num_sections,
        if_indexes, num_if_indexes);
    if(!psd_sections) {
        config_free(sections);
        return 1;
    }

    config_free(sections);

    char* buff = malloc(sizeof(char) * 15000);
    for(size_t i = 0; i < num_sections; i++) {
        sprintf(buff, "Section '%s' managed interfaces: ",
            psd_sections[i].section_name);
        for(size_t y = 0; y < psd_sections[i].num_interfaces_matched; y++) {
            char buff2[SIZE_T_MAX_DIGITS()];
            su_uint32_2_str(psd_sections[i].matched_interfaces[y]->index, buff2);
            if(y != psd_sections[i].num_interfaces_matched - 1) {
                strcat(buff2, ", ");
            }
            strcat(buff, buff2);
        }
        log_write(LL_INFO, "%s\n", buff);
    }
    free(buff);

    log_write(LL_INFO, "Creating ranges.\n");
    size_t num_root_ifhs;
    if_h* root_ifhs = ifh_create(psd_sections, num_sections,
        &num_root_ifhs);
    if(root_ifhs == NULL) {
        config_parse_free(psd_sections, num_sections);
        return 1;
    }

    log_write(LL_INFO, "Checking for cache: %s.\n", CLI_DB_PATH);
    int res = ifh_root_add_cli_db(&root_ifhs, num_root_ifhs, psd_sections,
        num_sections);

    if(res != CDB_SUCCESS) {
        if(res == CDB_ERROR_FAILED_TO_READ) {
            log_write(LL_INFO, "No cache file.\n");
        } else {
            config_parse_free(psd_sections, num_sections);
            ifh_free(root_ifhs, num_root_ifhs);
            log_write(LL_FATAL, "CDB had fatal error.\n");
            return 1;
        }
    } else {
        log_write(LL_INFO, "Got cache.\n");
    }

    log_write(LL_INFO, "Spawning compiler.\n");
    proc_h* compiler = pc_spawn();
    if(!compiler) {
        log_write(LL_INFO, "Compiler failed\n");
        return 1;
    }

    if(!proc_start(compiler)) {
        log_write(LL_FATAL, "Failed to start compiler process.\n");
        return 1;
    }

    ldata* ld = malloc(sizeof(ldata));
    ld->if_indexes = if_indexes;
    ld->num_if_indexes = num_if_indexes;
    ld->root_ifh = &root_ifhs;
    ld->num_root_ifhs = &num_root_ifhs;
    ld->psd_sections = psd_sections;
    ld->num_psd_sections = num_sections;
 
    log_write(LL_INFO, "Doing inital compile and attach\n");
    size_t num_unique_ifs;
    uint32_t* unique_ifs = ifh_get_uniq_idx(root_ifhs, num_root_ifhs,
        &num_unique_ifs);



    for(size_t i = 0; i < num_unique_ifs; i++) {
        const char* template_str = eebpf_create_template_2(root_ifhs,
            num_root_ifhs, unique_ifs[i]);
        if(!template_str) {
            log_write(LL_FATAL, "Failed to generate template.");
            return 1;
        }
        char* out = malloc(sizeof(char) * (MAX_ARG_LENGTH * MAX_ARG_COUNT));
        size_t out_size;
        size_t in_size = strlen(template_str);

        if(!pc_comp(template_str, in_size, out, &out_size)) {
            log_write(LL_FATAL, "Failed to compile.");
            return 1;
        }

        eebpf_detach_force(unique_ifs[i]);
        eebpf_load_and_attach_file(out, out_size, true, unique_ifs[i]);
    }

    log_write(LL_INFO, "Starting IPC server.\n");
    ipc_srv* srv = ipc_init(ld);
    if(!srv) {
        log_write(LL_FATAL, "The IPC server failed to start.\n");
        return 1;
    }

    log_write(LL_INFO, "IPC server started. Ready.\n");
    pthread_join(srv->thread, NULL);

    log_write(LL_INFO, "Cleaning up program.\n");
    ipc_srv_destroy(srv);
    proc_stop(compiler);
    free(compiler);
    free(ld);
    config_parse_free(psd_sections, num_sections);
    ifh_free(root_ifhs, num_root_ifhs);
    if_free((void*) if_indexes, num_if_indexes);
    sys_shutdown();
    log_close();
    return 0;
}

int main(int argc, char** argv) {
    return dxdp(argc, argv);
}
