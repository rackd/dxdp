#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "linux/limits.h"

extern char CONFIG_PATH[PATH_MAX];
extern char CLI_DB_PATH[PATH_MAX];
extern char COMP_PATH[PATH_MAX];
extern char LOG_FILE_PATH[PATH_MAX];
extern bool HAS_LOG_FILE;

/* Set paths and options */
void set_options();

#endif // OPTIONS_H_