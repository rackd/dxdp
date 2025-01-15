#ifndef BPF_HANDLER_H_
#define BPF_HANDLER_H_

#include "dxdp/if_handler.h"
#include <stddef.h>

extern const char COND_TEMPLATE_FIRST_IPV4[];
extern const char COND_TEMPLATE_ELSE_IPV4[];
extern const char COND_TEMPLATE_FIRST_IPV6[];
extern const char COND_TEMPLATE_ELSE_IPV6[];
extern const char EEBPF_CODE_TEMPLATE_START[2284];
extern const char EEBPF_CT_PROG_START[414];
extern const char EEBPF_COND_TEMPLATE_MIDDLE[201];
extern const char EEBPF_COND_TEMPLATE_END[72];

typedef struct eebpf_attachment {
    int if_index;
    int prog_fd;
}eebpf_attachment;

/* Attaches a xdp program to an interface.
 *
 * _path_or_buffer can be either binary in memory, or a path to a binary file.
 *
 * Specify by using bool _is_mem.
 */
bool eebpf_load_and_attach_file(const char* _path_or_buffer,
    size_t _len_or_null, bool _is_mem, int _if_index);

/* Given if handlers, create a C source code file for XDP BPF.
 */
const char* eebpf_create_template_2(if_h* _ifhs, size_t _num_ifhs,
    uint32_t if_index);

/* Detach XDP program from specified if index.
 */
bool eebpf_detach_force(int _if_index);

#endif // BPF_HANDLER_H_
