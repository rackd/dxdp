#include "dxdp/cantor_hash.h"

size_t cantor_pairing_hash(uint32_t x, int y) {
    size_t a = (size_t)x;
    size_t b = (size_t)(y - INT32_MIN);
    size_t hash = ((a + b) * (a + b + 1) / 2) + b;
    return hash;
}