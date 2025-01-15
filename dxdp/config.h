#ifndef CONFIG_H_
#define CONFIG_H_

#define CONFIG_BUFFER_SIZE 65536
#define CONFIG_MAX_ENTRY_PER_SECTION 15
#define CONFIG_MAX_LINE_LEN 256
#define CONFIG_MAX_KEY_LEN ((CONFIG_MAX_LINE_LEN-2) / 2)
#define CONFIG_MAX_VAL_LEN ((CONFIG_MAX_LINE_LEN-2) / 2)
#define CONFIG_MAX_SECTION_ID_LEN 128

#include "dxdp/interface.h"
#include "dxdp/iso3166.h"
#include <stddef.h>

typedef struct config_entry {;
    char key[CONFIG_MAX_KEY_LEN];
    char value[CONFIG_MAX_VAL_LEN];
} config_entry;

typedef struct config_section {
    char section_name[CONFIG_MAX_SECTION_ID_LEN];
    size_t num_entries;
    config_entry entries[CONFIG_MAX_ENTRY_PER_SECTION];
} config_section;

/* Read a dXDP config from disk at _path.
 * Returns array of config sections and number of sections via _num_sections or
 * NULL on failure.
 *
 * Must be freed by calling config_free.
 *
 * Prints error messages.
 */
config_section* config_read(const char* _path, size_t* _num_sections);

/* Frees an array of configuration sections
 */
void config_free(config_section* _config_sections);

/* Queries _key and writes value to _out_buff
 * Returns false if key does not exsist.
 *
 * Note, this does not take an config_section array, rather, a pointer to
 * a configuration seciton.
 */
bool config_get_val(config_section* p_config_section, const char* _key,
    char* _out_buff);

#endif // CONFIG_H_