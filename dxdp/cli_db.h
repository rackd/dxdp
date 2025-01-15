#ifndef CLI_DB_H_
#define CLI_DB_H_

#include "dxdp/db.h"
#include "dxdp/config.h"
#include "dxdp/config_parser.h"
#include "dxdp/if_handler.h"
#include <stddef.h>
#include <arpa/inet.h>

#define CDB_DELIM "-"
#define CDB_SUCCESS 0
#define CDB_ERROR_FAILED_TO_READ 1
#define CDB_ERROR_OTHER 2

typedef enum cdb_read_type {
    CDB_RT_ONLY_ADD = 0,
    CDB_RT_ONLY_RM = 1,
    CDB_RT_ONLY_ALL = 2,
}cdb_read_type;

typedef struct cli_db_line {
    char action[2];
    char type[2];
    char section_name[CONFIG_MAX_SECTION_ID_LEN];
    char low[INET6_ADDRSTRLEN];
    char high[INET6_ADDRSTRLEN];
} cli_db_line;

/*
 * Append _line to CLI database
 */
bool cdb_append(const char* _path, cli_db_line* _line);

/*
 * Read CLI database
 */
cli_db_line* cdb_read(const char* _path, size_t* _num_entries);

/*
 * Convert CLI database to if_h array
 */
if_h* cdb_2_ifh(cli_db_line* _cdb, size_t _num_lines,
    size_t* num_ifhs, config_psd_section* _sections, size_t _num_sections,
    cdb_read_type rtype);

/*
 * Remove _low - _high IP range from cli database at _path, reading and writing
 * to file.
 */
bool cdb_rm_range(const char* _path, const char* _low, const char* _high,
    bool is_ipv4);


#endif // CLI_DB_H_