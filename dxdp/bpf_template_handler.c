#include "dxdp/bpf_handler.h"
#include "dxdp/string_util.h"
#include "dxdp/itree_uint128.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

const char* eebpf_create_template_2(if_h* _ifhs, size_t _num_ifhs,
uint32_t if_index) {
    size_t sum_ipv4_ifs = 0;
    size_t sum_ipv6_ifs = 0;
    size_t sum_all;

    for(size_t i = 0; i < _num_ifhs; i++) {
        if (_ifhs[i].if_index == if_index) {
            if(_ifhs[i].num_ipv4s > 0) {
                sum_ipv4_ifs += 1;
            }

            if(_ifhs[i].num_ipv6s > 0) {
                sum_ipv6_ifs += 1;
            }
        }
    }

    sum_all = sum_ipv4_ifs + sum_ipv6_ifs;

    char** trees = malloc(sizeof(char*) * sum_all);
    if(!trees) {
        return NULL;
    }

    char** conds = malloc(sizeof(char*) * sum_all);
    if(!conds) {
        free(trees);
        return NULL;
    }

    char** names = malloc(sizeof(char*) * sum_all);
    if(!names) {
        free(trees);
        free(conds);
        return NULL;
    }

    char** num_nodes = malloc(sizeof(char*) * sum_all);
    if(!num_nodes) {
        free(trees);
        free(conds);
        free(names);
        return NULL;
    }

    char** max_layers = malloc(sizeof(char*) * sum_all);
    if(!max_layers) {
        free(trees);
        free(conds);
        free(names);
        free(num_nodes);
        return NULL;
    }

    char** negs = malloc(sizeof(char*) * sum_all);
    if(!negs) {
        free(trees);
        free(conds);
        free(names);
        free(num_nodes);
        free(max_layers);
        return NULL;
    }

    uint32_t* types = malloc(sizeof(uint32_t*) * sum_all);
    if(!types) {
        free(trees);
        free(conds);
        free(names);
        free(num_nodes);
        free(max_layers);
        free(negs);
        return NULL;
    }

    size_t* trees_lens = malloc(sizeof(size_t*) * sum_all);
    if(!trees_lens) {
        free(trees);
        free(conds);
        free(names);
        free(types);
        free(num_nodes);
        free(max_layers);
        free(negs);
        return NULL;
    }

    char buff[100000];
    size_t c = 0;

    for(size_t i = 0; i < _num_ifhs; i++) {
        if (_ifhs[i].if_index == if_index && _ifhs[i].num_ipv4s > 0) {

            uint32_itree* node = itree32_init(
                (uint32_range*) _ifhs[i].ipv4s,
                 _ifhs[i].num_ipv4s
            );
            sprintf(buff, "tree%zu", c);
            names[c] = strdup(buff);
            trees[c] = itree32_to_c_arr(node, buff);
            trees_lens[c] = strlen(trees[c]);
            itree32_free(node);


            su_sizet_2_str(_ifhs[i].num_ipv4s, buff);
            num_nodes[c] = strdup(buff);

            size_t ml = 2 * (size_t)(log2(_ifhs[i].num_ipv4s)) + 10;
            su_sizet_2_str(ml, buff);
            max_layers[c] = strdup(buff);

            if(c == 0) {
                types[c] = 0;
            } else {
                types[c] = 1;
            }

            if(_ifhs[i].mode == SECTION_MODE_WHITELIST) {
                strcpy(buff, "!");
            } else {
                strcpy(buff, "=");
            }
            negs[c] = strdup(buff);

            c++;
        }
    }

    size_t ipv6_start = c;

    for(size_t i = 0; i < _num_ifhs; i++) {
        if (_ifhs[i].if_index == if_index && _ifhs[i].num_ipv6s > 0) {
            uint128_itree* node_128 = itree128_init(
                (uint128_range*) _ifhs[i].ipv6s,
                _ifhs[i].num_ipv6s
            );
            sprintf(buff, "tree%zu", c);
            names[c] = strdup(buff);
            trees[c] = (char*) itree128_to_c_arr(node_128, buff);
            trees_lens[c] = strlen(trees[c]);
            itree128_free(node_128);

            su_sizet_2_str(_ifhs[i].num_ipv6s, buff);
            num_nodes[c] = strdup(buff);

            size_t ml = 2 * (size_t)(log2(_ifhs[i].num_ipv6s)) + 10;
            su_sizet_2_str(ml, buff);
            max_layers[c] = strdup(buff);

            if(c == sum_ipv4_ifs) {
                types[c] = 0;
            } else {
                types[c] = 1;
            }

            if(_ifhs[i].mode == SECTION_MODE_WHITELIST) {
                strcpy(buff, "!");
            } else {
                strcpy(buff, "=");
            }
            negs[c] = strdup(buff);

            c++;
        }
    }

    for(size_t i = 0; i < c; i++) {
        if(i < ipv6_start) {
            if(types[i] == 0) {
                sprintf(buff, COND_TEMPLATE_FIRST_IPV4,
                    names[i], max_layers[i], num_nodes[i], negs[i]);
            } else {
                sprintf(buff, COND_TEMPLATE_ELSE_IPV4,
                    names[i], max_layers[i], num_nodes[i], negs[i]);
            }

            conds[i] = strdup(buff);
        } else {
            if(types[i] == 0) {
                sprintf(buff, COND_TEMPLATE_FIRST_IPV6,
                    names[i], max_layers[i], num_nodes[i], negs[i]);
            } else {
                sprintf(buff, COND_TEMPLATE_ELSE_IPV6,
                    names[i], max_layers[i], num_nodes[i], negs[i]);
            }

            conds[i] = strdup(buff);
        }
    }

    size_t buff_total_size = 0;
    buff_total_size += sizeof(EEBPF_CODE_TEMPLATE_START);
    buff_total_size += sizeof(EEBPF_CT_PROG_START);
    buff_total_size += sizeof(EEBPF_COND_TEMPLATE_MIDDLE);
    buff_total_size += sizeof(EEBPF_COND_TEMPLATE_END);

    for(size_t i = 0; i < c; i++) {
        buff_total_size += trees_lens[i];
        buff_total_size += strlen(conds[i]);
    }

    size_t pos = 0;
    char* buffer = malloc(sizeof(char) * buff_total_size);
    if(!buffer) {
        for(size_t i = 0; i < c; i++) {
            free(trees[i]);
            free(names[i]);
            free(conds[i]);
            free(num_nodes[i]);
            free(max_layers[i]);
            free(negs[i]);
        }

        free(trees);
        free(names);
        free(conds);
        free(types);
        free(trees_lens);
        free(num_nodes);
        free(max_layers);
        free(negs);
        return NULL;
    }

    memset(buffer, 0, buff_total_size);

    memcpy(buffer + pos, EEBPF_CODE_TEMPLATE_START,
        sizeof(EEBPF_CODE_TEMPLATE_START) - 1);
    pos += sizeof(EEBPF_CODE_TEMPLATE_START) - 1;

    for(size_t i = 0; i < c; i++) {
        memcpy(buffer + pos, trees[i], trees_lens[i]);
        pos += trees_lens[i];
    }

    memcpy(buffer + pos, EEBPF_CT_PROG_START, sizeof(EEBPF_CT_PROG_START) - 1);
    pos += sizeof(EEBPF_CT_PROG_START) - 1;

    for(size_t i = 0; i < sum_ipv4_ifs; i++) {
        memcpy(buffer + pos, conds[i], strlen(conds[i]));
        pos += strlen(conds[i]);
    }

    memcpy(buffer + pos, EEBPF_COND_TEMPLATE_MIDDLE,
        sizeof(EEBPF_COND_TEMPLATE_MIDDLE) - 1);
    pos += sizeof(EEBPF_COND_TEMPLATE_MIDDLE) - 1;

    for(size_t i = sum_ipv4_ifs; i < c; i++) {
        memcpy(buffer + pos, conds[i], strlen(conds[i]));
        pos += strlen(conds[i]);
    }

    memcpy(buffer + pos, EEBPF_COND_TEMPLATE_END,
        sizeof(EEBPF_COND_TEMPLATE_END) - 1);
    pos += sizeof(EEBPF_COND_TEMPLATE_END) - 1;

    for(size_t i = 0; i < c; i++) {
        free(trees[i]);
        free(names[i]);
        free(conds[i]);
        free(num_nodes[i]);
        free(max_layers[i]);
        free(negs[i]);
    }

    free(trees);
    free(names);
    free(conds);
    free(types);
    free(trees_lens);
    free(num_nodes);
    free(max_layers);
    free(negs);
    return buffer;
}