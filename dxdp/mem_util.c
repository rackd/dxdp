#include "dxdp/mem_util.h"
#include <stdint.h>
#include <stdio.h>

int memcmp_bits(const void *__s1, const void *__s2, size_t bits) {
    const uint8_t *s1 = (const uint8_t*) __s1;
    const uint8_t *s2 = (const uint8_t*) __s2;

    size_t full_bytes = bits / 8;
    for (size_t i = 0; i < full_bytes; ++i) {
        if (s1[i] != s2[i]) {
            return s1[i] < s2[i] ? -1 : 1;
        }
    }

    size_t remaining_bits = bits % 8;
    if (remaining_bits != 0) {
        uint8_t mask = (1 << remaining_bits) - 1;
        uint8_t byte1 = s1[full_bytes] & mask;
        uint8_t byte2 = s2[full_bytes] & mask;

        if (byte1 != byte2) {
            return byte1 < byte2 ? -1 : 1;
        }
    }

    return 0;
}

bool arr_contains(void* src, size_t src_len, void* check, size_t size) {
    char* base = (char*) src;
    for (size_t i = 0; i < src_len; i++) {
        char* element = base + i * size;

        if (memcmp(element, check, size) == 0) {
            return true;
        }
    }
    return false;
}

int is_little_endian() {
    uint16_t num = 0x1;
    return (*(uint8_t*)&num) == 0x1;
}

__uint128_t swap_uint128(__uint128_t val) {
    __uint128_t swapped = 0;
    for (int i = 0; i < 16; ++i) {
        swapped = (swapped << 8) | (val & 0xFF);
        val >>= 8;
    }
    return swapped;
}

__uint128_t hton128(__uint128_t val) {
    if (is_little_endian()) {
        return swap_uint128(val);
    } else {
        return val;
    }
}

__uint128_t u8_2_128(const uint8_t arr[16]) {
    __uint128_t result = 0;
    if (is_little_endian()) {
        for (int i = 15; i >= 0; --i) {
            result = (result << 8) | arr[i];
        }
    } else {
        for (int i = 0; i < 16; ++i) {
            result = (result << 8) | arr[i];
        }
    }
    return hton128(result); // TODO: this works fine but is not developer friendly
}

