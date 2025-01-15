#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include "dxdp/config.h"
#include "dxdp/interface.h"
#include "dxdp/iso3166.h"
#include <stddef.h>
#include <linux/limits.h>

typedef enum {
    SECTION_TYPE_GEO = 0,
    SECTION_TYPE_RANGE = 1,
    SECTION_TYPE_NONE = 2
} section_type;

typedef enum {
    SECTION_MODE_WHITELIST = 0,
    SECTION_MODE_BLACKLIST = 1,
    SECTION_MODE_NONE = 2
} section_mode;

typedef enum {
    SECTION_DB_TYPE_IPV4 = 0,
    SECTION_DB_TYPE_IPV6 = 1,
    SECTION_DB_TYPE_IPV4_AND_IPV6 = 2,
    SECTION_DB_TYPE_NONE = 3
} db_type;

typedef enum {
    SECTION_ENABLED_TRUE = 0,
    SECTION_ENABLED_FALSE = 1,
    SECTION_ENABLED_NONE = 3
} section_enalbed;

typedef struct config_psd_section {
    char section_name[CONFIG_MAX_SECTION_ID_LEN];
    if_index** matched_interfaces;
    size_t num_interfaces_matched;
    section_type s_type;
    db_type d_type;
    section_mode mode;
    char ipv4_db_path[PATH_MAX];
    char ipv6_db_path[PATH_MAX];
    c_code c_codes[NUM_CODES];
    size_t num_c_codes;
    section_enalbed enabled;
} config_psd_section;

/* Given interfaces and array of configuration sections, create parsed sections
 * Must be freed by calling config_parse_free
*/
config_psd_section* config_parse(config_section* _sections,
    size_t _num_sections, const if_index* _if_indexes, size_t _num_if_indexes);

void config_parse_free(config_psd_section* _sections, size_t _num_sections);

/* Returns a pointer to config section given section name and sections.
*/
config_psd_section* section_name_2_psd(const char* _section_name,
    config_psd_section* _sections, size_t _num_sections);

#endif // CONFIG_PARSER_H_