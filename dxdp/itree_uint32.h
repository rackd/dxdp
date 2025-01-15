#ifndef ITREE_UINT32_H_
#define ITREE_UINT32_H_

#include "dxdp/itree.h"
#include <stdint.h>
#include <stddef.h>

typedef struct uint32_range {
    uint32_t low;
    uint32_t high;
} uint32_range;

typedef struct uint32_node {
    uint32_range data;
    struct uint32_node *left, *right, *parent;
    node_color color;
} uint32_node;

typedef struct uint32_itree {
    uint32_node *root;
} uint32_itree;

typedef struct uint32_serialized_node {
    uint32_t low;
    uint32_t high;
    int leftIndex;
    int rightIndex;
    int parentIndex;
    int color;
} uint32_serialized_node;

/* Create a interval red black tree given array of ranges. */
uint32_itree* itree32_init(const uint32_range* _rngs, size_t num_rngs);

void itree32_free(uint32_itree* _tree);

/* Convert a interval red black tree to C array format. */
char* itree32_to_c_arr(const uint32_itree *_tree, const char* _name);

#endif // ITREE_UINT32_H_