#ifndef ARGP_H_
#define ARGP_H_

#include "dxdp/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <linux/limits.h>

#define ARGS_MAX_IPS_PER_COMMAND 4096

typedef enum commands {
    COMMAND_NONE = 0,
    COMMAND_STATUS = 1,
    COMMAND_GEO_DB_UPDATE = 2,
    COMMAND_UPDATE = 3,
    COMMAND_DETACH = 4,
    COMMAND_QUIT = 5
 } commands;

typedef enum options {
    OPTION_NONE = 1,
    OPTION_SECTION_ID,
    OPTION_ADD_IP_STR,
    OPTION_ADD_IP_FILE,
    OPTION_REMOVE_IP_STR,
    OPTION_REMOVE_IP_FILE
} options;

typedef struct ip_str {
    char low[INET6_ADDRSTRLEN];
    char high[INET6_ADDRSTRLEN];
} ip_str;

typedef struct p_args {
    commands command;
    options option;
    char section_id[CONFIG_MAX_SECTION_ID_LEN];
    char ips_file[PATH_MAX];
    ip_str ips[ARGS_MAX_IPS_PER_COMMAND];
    size_t num_ips;
    int is_ipv4;
} p_args;

extern const char* usage_str;

void print_help();

bool parse_args(size_t _argc_in, char** _argv_in, p_args* _dest);

#endif // ARGP_H_