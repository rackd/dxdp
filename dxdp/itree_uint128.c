#include "dxdp/itree_uint128.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint128_node* create_node_128(uint128_range data, node_color color) {
    uint128_node* node = malloc(sizeof(uint128_node));
    if (!node) {
        return NULL;
    }
    node->data = data;
    node->left = node->right = node->parent = NULL;
    node->color = color;
    return node;
}

static void left_rotate_128(uint128_itree* tree, uint128_node* x) {
    uint128_node* y = x->right;
    x->right = y->left;
    if (y->left != NULL) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        tree->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

static void right_rotate_128(uint128_itree* tree, uint128_node* y) {
    uint128_node* x = y->left;
    y->left = x->right;
    if (x->right != NULL) {
        x->right->parent = y;
    }
    x->parent = y->parent;
    if (y->parent == NULL) {
        tree->root = x;
    } else if (y == y->parent->right) {
        y->parent->right = x;
    } else {
        y->parent->left = x;
    }
    x->right = y;
    y->parent = x;
}

static void insert_fixup_128(uint128_itree* tree, uint128_node* z) {
    while (z != tree->root && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            uint128_node* y = z->parent->parent->right;
            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate_128(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate_128(tree, z->parent->parent);
            }
        } else {
            uint128_node* y = z->parent->parent->left;
            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate_128(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate_128(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

static void rb_insert_128(uint128_itree* tree, uint128_range data) {
    uint128_node* z = create_node_128(data, RED);
    uint128_node* y = NULL;
    uint128_node* x = tree->root;

    while (x) {
        y = x;
        if (data.low < x->data.low)
            x = x->left;
        else
            x = x->right;
    }

    z->parent = y;
    if (!y)
        tree->root = z;
    else if (data.low < y->data.low)
        y->left = z;
    else
        y->right = z;

    insert_fixup_128(tree, z);
}

uint128_itree* itree128_init(const uint128_range* _ifhs, size_t _num_ifhs) {
    uint128_itree* tree = malloc(sizeof(uint128_itree));
    if (!tree) {
        return NULL;
    }

    tree->root = NULL;

    for (size_t i = 0; i < _num_ifhs; ++i) {
        rb_insert_128(tree, _ifhs[i]);
    }

    return tree;
}

static void free_tree(uint128_node* node) {
    if (!node)
        return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

void itree128_free(uint128_itree* _tree) {
    free_tree(_tree->root);
    free(_tree);
}

size_t count_nodes_128(uint128_node* node) {
    if (node == NULL) return 0;
    return 1 + count_nodes_128(node->left) + count_nodes_128(node->right);
}

void serialize_node_128(uint128_node* node, uint128_serialized_node* arr,
int* index, int parentIndex) {
    if (node == NULL) return;

    int currentIndex = *index;
    memcpy(arr[currentIndex].low, node->data.low,
        sizeof(arr[currentIndex].low));
    memcpy(arr[currentIndex].high, node->data.high,
        sizeof(arr[currentIndex].high));
    arr[currentIndex].parentIndex = parentIndex;
    arr[currentIndex].color = node->color == RED ? 0 : 1;

    if (node->left) {
        arr[currentIndex].leftIndex = *index + 1;
        (*index)++;
        serialize_node_128(node->left, arr, index, currentIndex);
    } else {
        arr[currentIndex].leftIndex = -1;
    }

    if (node->right) {
        arr[currentIndex].rightIndex = *index + 1;
        (*index)++;
        serialize_node_128(node->right, arr, index, currentIndex);
    } else {
        arr[currentIndex].rightIndex = -1;
    }
}

static void bytes_to_hex_string(const uint8_t bytes[16], char* hex_str) {
    int pos = 0;
    hex_str[pos++] = '{';

    for (int i = 0; i < 16; i++) {
        char hexbuff[3];
        sprintf(hexbuff, "%02X", bytes[i]);
        hex_str[pos++] = '0';
        hex_str[pos++] = 'x';
        memcpy(hex_str + pos, hexbuff, 2);
        pos+=2;

        if(i != 15) {
            hex_str[pos++] = ',';
        }
    }

    hex_str[pos++] = '}';
    hex_str[pos++] = '\0';
}

const char* itree128_to_c_arr(const uint128_itree* _tree, const char* _name) {
    size_t num_nodes = count_nodes_128(_tree->root);
    uint128_serialized_node* arr =
        malloc(sizeof(uint128_serialized_node) * num_nodes);
    if (!arr) {
        return NULL;
    }

    int index = 0;
    int lines_per_node = 700;

    serialize_node_128(_tree->root, arr, &index, -1);

    size_t estimated_size = lines_per_node * num_nodes;
    char* result = malloc(estimated_size);
    if (result == NULL) {
        free(arr);
        return NULL;
    }

    int written = sprintf(result, "uint128_serialized_node %s[%zu] = {\n",
        _name, num_nodes);
    if (written < 0) {
        free(arr);
        free(result);
        return NULL;
    }

    char* current_pos = result + written;
    size_t remaining_size = estimated_size - written;

    for (size_t i = 0; i < num_nodes; i++) {
        char low_str[100];
        char high_str[100];
        bytes_to_hex_string(arr[i].low, low_str);
        bytes_to_hex_string(arr[i].high, high_str);

        written = snprintf(current_pos, remaining_size,
            "    { %s, %s , %d, %d, %d, %d },\n",
            low_str,
            high_str,
            arr[i].leftIndex,
            arr[i].rightIndex,
            arr[i].parentIndex,
            arr[i].color);

        if (written < 0 || (size_t)written >= remaining_size) {
            return NULL;
        }

        current_pos += written;
        remaining_size -= written;
    }

    strcat(current_pos, "};\n");
    free(arr);
    return result;
}