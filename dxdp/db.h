#ifndef DB_H_
#define DB_H_

#include "dxdp/iso3166.h"
#include "dxdp/config_parser.h"
#include <stdint.h>
#include <pthread.h>
#include <netinet/in.h>

#define DB_LINE_BUFFER_LENGTH 1024

typedef enum {
    FILE_TYPE_GEO_IPV4_CSV = 0,
    FILE_TYPE_IPV4_CSV     = 1,
    FILE_TYPE_GEO_IPV6_CSV = 2,
    FILE_TYPE_IPV6_CSV     = 3,
    FILE_TYPE_NONE         = 4
} file_type;

typedef struct db_ipv4 {
    c_code cc;
    uint32_t low;
    uint32_t high;
} db_ipv4;

typedef struct db_ipv6 {
    c_code cc;
    uint8_t low[16];
    uint8_t high[16];
} db_ipv6;

typedef struct db_s {
    file_type f_type;
    db_ipv4* ipv4s;
    size_t num_ipv4;
    db_ipv6* ipv6s;
    size_t num_ipv6;
} db_s;

/* Initalizes and returns a database structure.
 * This structure must be freed by calling db_free().
 * Returns NULL on failure.
 */
db_s* db_init();

/* Takes a pointer to a configuration section, then, reads the 1-2 database
 * files in said section. Then appends that information to a _db structure.
 * Returns true on success or  false on failure.
 */
bool db_read(db_s* _db, config_psd_section* _p_section);

/* Frees a database structure.
*/
void db_free(db_s* _db);
#endif // DB_H_