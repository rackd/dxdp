#ifndef MEMUTIL_H_
#define MEMUTIL_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int memcmp_bits(const void *__s1, const void *__s2, size_t bits);

bool arr_contains(void* src, size_t src_len, void* check, size_t size);

__uint128_t u8_2_128(const uint8_t arr[16]);

__uint128_t hton128(__uint128_t val);

#endif // MEMUTIL_H_