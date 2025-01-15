#ifndef HASH_MAP_H_
#define HASH_MAP_H_

#include <stddef.h>

#define HM_INIT_SIZE_DEFAULT 4096

typedef struct hash_map {
    size_t* keys;
    size_t* values;
    size_t capacity;
    size_t size;
} hash_map;

/* Initalizes a new hashmap with an inital capacity.
 * Returns handle or NULL on failure.
 * Must be freed using hm_free
*/
hash_map* hm_new(size_t _initial_capacity);

void hm_insert(hash_map* _hm, size_t _key, size_t _value);

void hm_remove(hash_map* _hm, size_t _key);

bool hm_exists(hash_map* _hm, size_t _key);

void hm_free(hash_map* _hm);

size_t hm_value_for(hash_map* _hm, size_t _key);

#endif // HASH_MAP_H_