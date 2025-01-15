#ifndef FILE_UTIL_H_
#define FILE_UTIL_H_

#include <stddef.h>

/* Read a file, splitting by new line char.
 * Returns a array of strings(lines) and writes num of lines to _num_lines.
 * _buffer_size is line buffer size, stored on the stack.
 * Must be freed by calling file_read_free()
 */
char** file_read(const char* _path, size_t _buffer_size, size_t* _num_lines);

/* Free string array called by file_read()
*/
void file_read_free(char** _lines, size_t _num_lines);

#endif // FILE_UTIL_H_