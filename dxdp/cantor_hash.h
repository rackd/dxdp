#ifndef CANTOR_H_
#define CANTOR_H_

#include <stddef.h>
#include <stdint.h>

/* Generate cantor hash using a uint32_t and int
 */
size_t cantor_pairing_hash(uint32_t x, int y);

#endif // CANTOR_H_