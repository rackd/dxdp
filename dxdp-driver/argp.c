#include "dxdp-driver/argp.h"
#include "dxdp/string_util.h"
#include <stdio.h>
#include <string.h>

const char* usage_str =
    "Usage: dxdp [command] [options]\n"
    "\n"
    "Commands:\n"
    "\tstatus:\n"
    "\t\tDisplays current configuration status and interface status.\n"
    "\tgeo-update\n"
    "\t\tDownload updated geo database\n"
    "\tupdate\n"
    "\t\tUpdate rules for a section\n"
    "\tdetach\n"
    "\t\tDetach program(s) from kernel\n"
    "\tquit\n"
    "\t\tStop the daemon.\n"
    "\n"
    "Options:\n"
    "\t--section-id, -s <section id>\n"
    "\t\tSpecifies the section ID on which the command operates. If section ID is all, then, options are applied to all configuration sections.\n"
    "\t--add-ips, -a <ips>\n"
    "\t\tAdds one or more ips to the specified section rules\n"
    "\t\tIPs are separated by commas ','.\n"
    "\t\tIP ranges can be denoted by using '-' between two ip addresses.\n"
    "\t\tIPs added here are processed based on the rules in this section.\n"
    "\t--add-ips-from-file, -af <path>\n"
    "\t\tSame as --add-ips, except, reads IPs from a file instead of cli.\n"
    "\t\tEach line in the file should contain a single IP or an IP range separated by '-'\n"
    "\t--remove-ips, -r <ips>\n"
    "\t\tRemove one or more ips from the specified section separated by commas.\n"
    "\t\tIPs added here are excluded from processing of a given section's rules.\n"
    "\t\tThe format for listing IPs or ranges is the same as `--add-ips`.\n"
    "\t--remove-ips-from-file, -rf <path>\n"
    "\t\tRemoves IPs or IP ranges from the specified section by reading them from a file.\n"
    "\t\tSee '--add-ips-from-file'\n"
    "\t--remove-ips-from-file, -rf <path>\n"
    "\t\tRemoves IPs or IP ranges from the specified section by reading them from a file.\n"
    "\n"
    "Examples:\n"
    "\tdxdp update -s Section1 -a 10.0.0.1\n"
    "\tdxdp update -s Section2 -r 192.168.1.12-192.168.1.24\n"
    "\tdxdp update -s Section3 -a 192.168.1.12-192.168.1.24,10.0.0.5,192.168.10.12-192.168.10.14\n"
    "\tdxdp update -s BannedIPS -r ff06::c3\n"
    "\tdxdp update -s WhitelistedIPs -a ff06::c3-ff06::c7\n"
    "\tdxdp update -s WhitelistedIPs -a ff06::c3-ff06::c7,ff06::c15-ff06::c22\n"
    "\tdxdp update -s Blocked -af ~/my_ips/new-bad-list.txt\n"
    "\tdxdp status\n"
    "\tdxdp quit\n";

bool b_strcmp(const char* _s1, const char* _s2) {
    if(strcmp(_s1, _s2) == 0) {
        return true;
    }
    return false;
}

bool b_strncmp(const char* _s1, const char* _s2, size_t _n) {
    if(strncmp(_s1, _s2, _n) == 0) {
        return true;
    }
    return false;
}

options str_2_option(const char* _opt) {
    if(b_strcmp(_opt, "--section-id") || b_strcmp(_opt, "-s")) {
        return OPTION_SECTION_ID;
    } else if(b_strcmp(_opt, "--add-ips") || b_strcmp(_opt, "-a")) {
        return OPTION_ADD_IP_STR;
    } else if(b_strcmp(_opt, "--add-ips-from-file") || b_strcmp(_opt, "-af")) {
        return OPTION_ADD_IP_FILE;
    } else if(b_strcmp(_opt, " --remove-ips") || b_strcmp(_opt, "-r")) {
        return OPTION_REMOVE_IP_STR;
    } else if(b_strcmp(_opt, "--remove-ips-from-file") || b_strcmp(_opt, "-rf")) {
        return OPTION_REMOVE_IP_FILE;
    } else {
        return OPTION_NONE;
    }
}

bool parse_args(size_t _argc_in, char** _argv_in, p_args* _dest) {
    _dest->option = OPTION_NONE;
    _dest->num_ips = 0;
    _dest->command = COMMAND_NONE;
    _dest->is_ipv4 = true;

    if(_argc_in <= 1) {
        fprintf(stderr, "Too few arguments.\n");
        print_help();
        return false;
    }

    if(_argc_in > 6) {
        fprintf(stderr, "Too many arguments.\n");
        print_help();
        return false;
    }

    if(b_strcmp(_argv_in[1], "gdb-update")) {
        _dest->command = COMMAND_GEO_DB_UPDATE;
    } else if (b_strcmp(_argv_in[1], "update")) {
        _dest->command = COMMAND_UPDATE;
    } else if (b_strcmp(_argv_in[1], "status")) {
        _dest->command = COMMAND_STATUS;
    } else if (b_strcmp(_argv_in[1], "detach")) {
        _dest->command = COMMAND_DETACH;
    } else if (b_strcmp(_argv_in[1], "quit")) {
        _dest->command = COMMAND_QUIT;
    } else {
        fprintf(stderr, "Command '%s' is not valid.\n", _argv_in[1]);
        print_help();
        return false;
    }

    bool has_section_id = false;
    for(size_t i = 2; i <= _argc_in - 1; i+=2) {
        options opt = str_2_option(_argv_in[i]);
        if(opt == OPTION_NONE) {
            fprintf(stderr, "Invalid option. '%s'.\n", _argv_in[i]);
            print_help();
            return false;
        }

        if(i + 1 >= _argc_in) {
            fprintf(stderr, "Missing data argument for '%s'.\n", _argv_in[i]);
            print_help();
            return false;
        }

        const char* option_arg = _argv_in[i+1];

        switch(opt) {
            case OPTION_SECTION_ID:
                if(has_section_id) {
                    fprintf(stderr, "Only one section id allowed per command.\n");
                    print_help();
                    return false;
                }

                if(strlen(option_arg) >= CONFIG_MAX_SECTION_ID_LEN) {
                    fprintf(stderr, "Invalid section ID (too long).\n");
                    print_help();
                    return false;
                }

                strcpy(_dest->section_id, option_arg);
                has_section_id = true;
            break;

            case OPTION_ADD_IP_STR:
            case OPTION_REMOVE_IP_STR:
                size_t num_ips;
                const char** ips = su_str_split(option_arg, ',', &num_ips);
                _dest->option = OPTION_ADD_IP_STR;

                for(size_t y = 0; y < num_ips; y++) {
                    size_t num_lh;
                    const char** ip_low_high = su_str_split(ips[y], '-', &num_lh);

                    if(y == 0) {
                        if(strlen(ip_low_high[0]) > INET_ADDRSTRLEN || strchr(ips[y], ':') != NULL) {
                            _dest->is_ipv4 = false;
                        } else {
                            _dest->is_ipv4 = true;
                        }
                    }

                    if(num_lh == 1) {
                        if(strlen(ips[y]) > INET6_ADDRSTRLEN - 1) {
                            fprintf(stderr, "%s\nThe above IP address range is in the incorrect format.\nIt should contain two addresses (low and high) seperated by '-'.\n", option_arg);
                            su_free_split(ip_low_high, num_lh);
                            su_free_split(ips, num_ips);
                            return false;
                        }

                        if(_dest->is_ipv4) {
                            if(strchr(ips[y], ':') != NULL || strlen(ips[0]) > INET_ADDRSTRLEN) {
                                fprintf(stderr, "Do not mix and match ipv4/ipv6 addresses.\n");
                                return false;
                            }
                        } else {
                            if(strchr(ips[y], ':') == NULL) {
                                fprintf(stderr, "Do not mix and match ipv4/ipv6 addresses.\n");
                            }
                        }

                        strncpy(_dest->ips[y].low, ips[y], INET6_ADDRSTRLEN);
                        strncpy(_dest->ips[y].high, ips[y], INET6_ADDRSTRLEN);
                    } else if (num_lh == 2) {
                        if(strlen(ip_low_high[0]) > INET6_ADDRSTRLEN
                        || strlen(ip_low_high[1]) > INET6_ADDRSTRLEN) {
                            fprintf(stderr, "%s\nThe above IP address range is in the incorrect format.\nIt should contain two addresses (low and high) seperated by '-'.\n", option_arg);
                            su_free_split(ip_low_high, num_lh);
                            su_free_split(ips, num_ips);
                            return false;
                        }

                        strncpy(_dest->ips[y].low, ip_low_high[0],
                            INET6_ADDRSTRLEN);
                        strncpy(_dest->ips[y].high, ip_low_high[1],
                            INET6_ADDRSTRLEN);
                    } else {
                        fprintf(stderr, "%s\nThe above IP address range is in the incorrect format.\nIt should contain two addresses (low and high) seperated by '-'.\n", option_arg);
                        su_free_split(ip_low_high, num_lh);
                        su_free_split(ips, num_ips);
                        return false;
                    }
                    _dest->ips[y].low[INET6_ADDRSTRLEN - 1] = '\0';
                    _dest->ips[y].high[INET6_ADDRSTRLEN - 1] = '\0';
                    su_free_split(ip_low_high, num_lh);
                }

                _dest->num_ips = num_ips;
                _dest->option = opt;

                su_free_split(ips, num_ips);
            break;

            case OPTION_ADD_IP_FILE:
            case OPTION_REMOVE_IP_FILE:
                _dest->option = opt;
                _dest->num_ips = 0;
                strncpy(_dest->ips_file,  _argv_in[5], PATH_MAX);
                _dest->ips_file[PATH_MAX - 1] = '\0';
            break;

            default:
                return false;
            break;
        }


    }

    if((_dest->command != COMMAND_DETACH && _dest->command != COMMAND_QUIT
    && _dest->command != COMMAND_STATUS) && !has_section_id) {
        fprintf(stderr, "Command requires a section ID.\n");
        print_help();
        return false;
    }

    return true;
}

void print_help() {
    fprintf(stdout, "%s\n", usage_str);
}