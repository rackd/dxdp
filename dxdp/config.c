#include "dxdp/config.h"
#include "dxdp/string_util.h"
#include "dxdp/file_util.h"
#include "dxdp/log.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

config_section* config_read(const char* _path, size_t* _num_sections) {
    size_t num_lines;
    char** lines = file_read(_path, CONFIG_BUFFER_SIZE, &num_lines);
    if(!lines) {
        log_write(LL_FATAL, "Failed to open/read config file\n.");
        log_write(LL_FATAL, "Perhaps you did not create a config yet?\n");
        return NULL;
    }

    size_t num_sections = 0;
    for(size_t i = 0; i < num_lines; i++) {
        char* line = lines[i];
        su_trim_line(line);
        if (strlen(line) == 0) continue;

        if (line[0] == '[') {
            num_sections++;
        }
    }

    config_section* sections = malloc(sizeof(config_section) * num_sections);
    if(!sections) {
        return NULL;
    }

    size_t section_i = 0;
    bool has_section = false;

    for(size_t i = 0; i < num_lines; i++) {
        char* line = lines[i];
        su_trim_line(line);
        if (strlen(line) == 0) continue;

        if (line[0] == '[') {
            char* end_brace = strchr(line, ']');
            if (!end_brace) {
                log_write(LL_FATAL,
                    "CONFIG ERROR: ']' missing in line: '%s'.\n",
                    line);
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }
            *end_brace = '\0';

            char* section_name = line + 1;
            size_t len = strlen(section_name);
            if(len <= 0) {
                log_write(LL_FATAL,
                    "CONFIG ERROR: Section name is blank on line '%s'.\n",
                    line);
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }

            if((len+1) >= CONFIG_MAX_SECTION_ID_LEN) {
                log_write(LL_FATAL,
                    "CONFIG ERROR: Section name too long on line '%s'.\n",
                    line);
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }

            if(has_section) {
                section_i++;
            } else {
                has_section = true;
            }

            strcpy(sections[section_i].section_name, section_name);
            sections[section_i].num_entries = 0;
        } else {
            if(!has_section) {
                log_write(LL_FATAL,
                    "CONFIG ERROR: Config entry included before "
                    "defining section.\n");
                file_read_free(lines, num_lines);
                free(sections);
                return NULL;
            }

            char* delimiter = strchr(line, '=');
            if (!delimiter) {
                log_write(LL_FATAL,
                    "CONFIG ERROR: '=' missing from line: '%s'.\n", line);
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }
            *delimiter = '\0';
            char* key = line;
            char* value = delimiter + 1;

            if((strlen(key) + 1) >= CONFIG_MAX_KEY_LEN) {
                log_write(LL_FATAL, "CONFIG ERROR: Key '%s' too long.\n",
                    line);
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }

            if((strlen(value) + 1) >= CONFIG_MAX_VAL_LEN) {
                log_write(LL_FATAL, "CONFIG ERROR: Val '%s' too long.\n",
                    line);
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }

            strcpy(
                sections[section_i].entries[sections[section_i].num_entries].key,
                key);

            strcpy(
                sections[section_i].entries[sections[section_i].num_entries++].value,
                value
            );

            if(sections[section_i].num_entries >=
                CONFIG_MAX_ENTRY_PER_SECTION) {
                log_write(LL_FATAL, "CONFIG ERROR: Too many entries.\n");
                file_read_free(lines, num_lines);
                config_free(sections);
                return NULL;
            }
        }
    }

    file_read_free(lines, num_lines);

    *_num_sections = num_sections;
    return sections;
}

void config_free(config_section* _config_sections) {
    free(_config_sections);
}

bool config_get_val(config_section* p_config_section, const char* _key,
    char* _out_buff) {
    for(size_t i = 0; i < p_config_section->num_entries; i++) {
        if(strcmp(p_config_section->entries[i].key, _key) == 0) {
            strcpy(_out_buff, p_config_section->entries[i].value);
            return true;
        }
    }

    return false;
}