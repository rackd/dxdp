#ifndef PROC_COMP_H_
#define PROC_COMP_H_

#include "dxdp/proc.h"
#include <stddef.h>

/* Create new compiler process and return handle. */
proc_h* pc_spawn();

/* Takes c source code, spawns a compiler process, then, writes binary
 * to _out_buff, alongside bin size to _bin_size.
 */
bool pc_comp(const char* _in_source, size_t _in_size,
    char* _out_buff, size_t* _bin_size);

#endif // PROC_COMP_H_