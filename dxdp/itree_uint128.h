#ifndef ITREE_UINT128_H_
#define ITREE_UINT128_H_

#include "dxdp/itree.h"
#include <stdint.h>
#include <stddef.h>

typedef struct uint128_range {
    uint8_t low[16];
    uint8_t high[16];
} uint128_range;

typedef struct uint128_node {
    uint128_range data;
    struct uint128_node *left, *right, *parent;
    node_color color;
} uint128_node;

typedef struct uint128_itree {
    uint128_node *root;
} uint128_itree;

typedef struct uint128_serialized_node {
    uint8_t low[16];
    uint8_t high[16];
    int leftIndex;
    int rightIndex;
    int parentIndex;
    int color;
} uint128_serialized_node;

/* Create a interval red black tree given array of ranges. */
uint128_itree* itree128_init(const uint128_range* _ifhs, size_t _num_ifhs);

void itree128_free(uint128_itree* tree);

/* Convert a interval red black tree to C array format. */
const char* itree128_to_c_arr(const uint128_itree *tree, const char* name);

#endif // ITREE_UINT128_H_
