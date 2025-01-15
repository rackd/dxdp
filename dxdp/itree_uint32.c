#include "dxdp/itree_uint32.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint32_node* create_node(uint32_range data, node_color color) {
    uint32_node* node = malloc(sizeof(uint32_node));
    if (!node) {
        return NULL;
    }
    node->data = data;
    node->left = node->right = node->parent = NULL;
    node->color = color;
    return node;
}

static void left_rotate(uint32_itree* tree, uint32_node* x) {
    uint32_node* y = x->right;
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

static void right_rotate(uint32_itree* tree, uint32_node* y) {
    uint32_node* x = y->left;
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

static void insert_fixup(uint32_itree* tree, uint32_node* z) {
    while (z != tree->root && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            uint32_node* y = z->parent->parent->right;
            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            uint32_node* y = z->parent->parent->left;
            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

static void rb_insert(uint32_itree* tree, uint32_range data) {
    uint32_node* z = create_node(data, RED);
    uint32_node* y = NULL;
    uint32_node* x = tree->root;

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

    insert_fixup(tree, z);
}

uint32_itree* itree32_init(const uint32_range* _rngs, size_t num_rngs) {
    uint32_itree* tree = malloc(sizeof(uint32_itree));
    if (!tree) {
        return NULL;
    }

    tree->root = NULL;

    for (size_t i = 0; i < num_rngs; ++i) {
        rb_insert(tree, _rngs[i]);
    }

    return tree;
}

static void free_tree(uint32_node* node) {
    if (!node)
        return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

void itree32_free(uint32_itree* _tree) {
    free_tree(_tree->root);
    free(_tree);
}

size_t count_nodes(uint32_node* node) {
    if (node == NULL) return 0;
    return 1 + count_nodes(node->left) + count_nodes(node->right);
}

void serialize_node(uint32_node* node, uint32_serialized_node *arr,
int* index, int parentIndex) {
    if (node == NULL) return;

    int currentIndex = *index;
    arr[currentIndex].low= node->data.low;
    arr[currentIndex].high = node->data.high;
    arr[currentIndex].parentIndex = parentIndex;
    arr[currentIndex].color = node->color == RED ? 0 : 1;

    if (node->left) {
        arr[currentIndex].leftIndex = *index + 1;
        (*index)++;
        serialize_node(node->left, arr, index, currentIndex);
    } else {
        arr[currentIndex].leftIndex = -1;
    }

    if (node->right) {
        arr[currentIndex].rightIndex = *index + 1;
        (*index)++;
        serialize_node(node->right, arr, index, currentIndex);
    } else {
        arr[currentIndex].rightIndex = -1;
    }
}

char* itree32_to_c_arr(const uint32_itree* _tree, const char* _name) {
    size_t num_nodes = count_nodes(_tree->root);
    uint32_serialized_node* arr =
        malloc(sizeof(uint32_serialized_node) * num_nodes);
    if (!arr) {
        return NULL;
    }

    int index = 0;
    int len_per_node = 80;
    serialize_node(_tree->root, arr, &index, -1);

    size_t estimated_size = (len_per_node * num_nodes) + 100;
    char* result = malloc(estimated_size);
    if (result == NULL) {
        return NULL;
    }

    sprintf(result, "uint32_serialized_node %s[%zu] = {\n", _name, num_nodes);
    for (size_t i = 0; i < num_nodes; i++) {
        char node_repr[len_per_node + 100]; //TODO: verify
        sprintf(node_repr, "    { %u, %u , %d, %d, %d, %d },\n",
                arr[i].low,
                arr[i].high,
                arr[i].leftIndex,
                arr[i].rightIndex,
                arr[i].parentIndex,
                arr[i].color);
        strcat(result, node_repr);
    }
    strcat(result, "};\n");
    free(arr);
    return result;
}