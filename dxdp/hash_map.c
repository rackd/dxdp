#include "dxdp/hash_map.h"
#include <stdlib.h>
#include <string.h>

size_t _hash(size_t key, size_t capacity) {
    return key % capacity;
}

void _hm_rehash(hash_map *_hm, size_t _new_capacity) {
    size_t* old_keys = _hm->keys;
    size_t* old_values = _hm->values;
    size_t old_capacity = _hm->capacity;

    _hm->keys = calloc(_new_capacity, sizeof(size_t));
    if(!_hm->keys) {
        _hm->keys = old_keys;
        _hm->values = old_values;
        return;
    }

    _hm->values = calloc(_new_capacity, sizeof(size_t));
    if(!_hm->values) {
        free(_hm->keys);
        _hm->keys = old_keys;
        _hm->values = old_values;
        return;
    }

    _hm->capacity = _new_capacity;
    _hm->size = 0;

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_keys[i] != 0) {
            hm_insert(_hm, old_keys[i], old_values[i]);
        }
    }

    free(old_keys);
    free(old_values);
}

void _hm_check_grow(hash_map* _hm) {
    float load_factor = _hm->size / (float) _hm->capacity;
    if (load_factor > 0.75) {
        _hm_rehash(_hm, _hm->capacity * 2);
    }
}

hash_map* hm_new(size_t _initial_capacity) {
    hash_map* hm = malloc(sizeof(hash_map));
    if (hm == NULL) {
        return NULL;
    }
    hm->capacity = _initial_capacity;
    hm->size = 0;

    hm->keys = calloc(hm->capacity, sizeof(size_t));
    if(!hm->keys) {
        free(hm);
        return NULL;
    }

    hm->values = calloc(hm->capacity, sizeof(size_t));
    if(!hm->values) {
        free(hm->keys);
        free(hm);
        return NULL;
    }

    return hm;
}

void hm_insert(hash_map* _hm, size_t _key, size_t _value) {
    _hm_check_grow(_hm);
    size_t index = _hash(_key, _hm->capacity);
    while (_hm->keys[index] != 0 && _hm->keys[index] != _key) {
        index = (index + 1) % _hm->capacity;
    }
    if (_hm->keys[index] == 0) {
        _hm->size++;
    }
    _hm->keys[index] = _key;
    _hm->values[index] = _value;
}

bool hm_exists(hash_map* _hm, size_t _key) {
    size_t index = _hash(_key, _hm->capacity);
    while (_hm->keys[index] != 0) {
        if (_hm->keys[index] == _key) {
            return true;
        }
        index = (index + 1) % _hm->capacity;
    }
    return false;
}

void hm_remove(hash_map* _hm, size_t _key) {
    size_t index = _hash(_key, _hm->capacity);
    while (_hm->keys[index] != 0) {
        if (_hm->keys[index] == _key) {
            _hm->keys[index] = 0;
            _hm->values[index] = 0;
            _hm->size--;
            return;
        }
        index = (index + 1) % _hm->capacity;
    }
}

size_t hm_value_for(hash_map* _hm, size_t _key) {
    size_t index = _hash(_key, _hm->capacity);
    while (_hm->keys[index] != 0) {
        if (_hm->keys[index] == _key) {
            return _hm->values[index];
        }
        index = (index + 1) % _hm->capacity;
    }
    return 0;
}

void hm_free(hash_map* _hm) {
    free(_hm->keys);
    free(_hm->values);
    free(_hm);
}