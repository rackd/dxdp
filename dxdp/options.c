#include "dxdp/options.h"
#include "dxdp/sysutil.h"
#include <stdlib.h>
#include <string.h>

char CONFIG_PATH[PATH_MAX] = {0};
char CLI_DB_PATH[PATH_MAX] = {0};
char COMP_PATH[PATH_MAX] = {0};
char LOG_FILE_PATH[PATH_MAX] = {0};
bool HAS_LOG_FILE = false;

void set_options() {
    char* value = getenv("CONFIG_PATH");

    if(value) {
        strcpy(CONFIG_PATH, value);
    } else {
        strcpy(CONFIG_PATH, DEFAULT_CONFIG_PATH);
    }

    value = getenv("CLI_DB_PATH");
    if(value) {
        strcpy(CLI_DB_PATH, value);
    } else {
        strcpy(CLI_DB_PATH, DEFAULT_CDB_PATH);
    }

    value = getenv("COMP_PATH");
    if(value) {
        strcpy(COMP_PATH, value);
    } else {
        strcpy(COMP_PATH, DEFAULT_COMPILER_PATH);
    }

    value = getenv("LOG_FILE_PATH");
    if(value) {
        HAS_LOG_FILE = true;
        strcpy(LOG_FILE_PATH, value);
    }
}