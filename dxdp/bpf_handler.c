#include "dxdp/bpf_handler.h"
#include "dxdp/log.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/if_link.h>

bool eebpf_load_and_attach_file(const char* _path_or_buffer,
size_t _len_or_null, bool _is_mem, int _if_index) {
    int err;
    struct bpf_object* obj;

    if(!_is_mem) {
        obj = bpf_object__open_file(_path_or_buffer, NULL);
    } else {
        obj = bpf_object__open_mem(_path_or_buffer, _len_or_null, NULL);
    }

    if (!obj) {
        return false;
    }

    err = bpf_object__load(obj);
    if (err) {
        bpf_object__close(obj);
        return false;
    }

    struct bpf_program* prog = bpf_object__next_program(obj, NULL);
    if (!prog) {
        bpf_object__close(obj);
        return false;
    }

    int prog_fd = bpf_program__fd(prog);

    err = bpf_xdp_attach(_if_index, prog_fd, XDP_FLAGS_UPDATE_IF_NOEXIST, NULL);
    if (err) {
        bpf_object__close(obj);
        return false;
    }

    bpf_object__close(obj);

    return true;
}

bool eebpf_detach_force(int _if_index) {
    return !bpf_xdp_detach(_if_index, 0, NULL);
}
