#ifndef ISO3166_H_
#define ISO3166_H_

#include <stdint.h>

/* ISO3166 country code hash table */

#define CODE_LENGTH 3
#define NUM_CODES 249
#define HASH_TABLE_SIZE 700

typedef int c_code;

extern const char iso3166_codes[NUM_CODES][CODE_LENGTH];

void cc_init();

c_code cc_to_idx(const char* code);

const char* cc_from_idx(c_code index);

#endif // ISO3166_H_