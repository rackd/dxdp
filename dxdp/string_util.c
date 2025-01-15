#include "dxdp/string_util.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

const char** su_str_split(const char* _str, const char _delim, size_t* _arr_len){
    size_t num = 0;
    char* tmp = strdup(_str);
    char delim[2] = {_delim, '\0'};

    for (int i = 0; tmp[i]; i++) {
        if (tmp[i] == _delim) {
            num++;
        }
    }

    const char** res = malloc(sizeof(char*) * (num + 1));
    char* token = strtok(tmp, delim);
    size_t i = 0;
    while (token != NULL) {
        res[i++] = strdup(token);
        token = strtok(NULL, delim);
    }

    free(tmp);
    *_arr_len = i;
    return res;
}

void su_free_split(const char** _strs, size_t _num_strs) {
    for (size_t i = 0; i < _num_strs; i++) {
        free((void*) _strs[i]);
    }

    free((void**) _strs);
}

void su_trim_line(char* _line) {
    char* hash_ptr = strchr(_line, '#');
    if (hash_ptr) {
        *hash_ptr = '\0';
    }
    char* d = _line;
    do {
        while (*d == ' ' || *d == '\t' || *d == '\n' || *d == '\r') {
            ++d;
        }
    } while ((*_line++ = *d++));
}

void su_to_lower(char* _str) {
    size_t len = strlen(_str);
    for(size_t i = 0; i < len; i++){
        _str[i] = tolower(_str[i]);
    }
}

void su_to_upper(char* _str) {
    size_t len = strlen(_str);
    for(size_t i = 0; i < len; i++){
        _str[i] = toupper(_str[i]);
    }
}
char* su_find_replace(const char* _str, const char* _find, const char* _replace) {
    size_t find_len = strlen(_find);
    size_t replace_len = strlen(_replace);
    size_t len_diff = replace_len - find_len;

    char *pos = (char*) _str;
    size_t new_str_len = strlen(_str);
    while ((pos = strstr(pos, _find))) {
        new_str_len += len_diff;
        pos += find_len;
    }

    char *new_str = malloc(new_str_len + 1);
    if (!new_str) {
        return NULL;
    }
    new_str[0] = '\0';

    char *last_pos = (char*) _str;
    pos = (char*) _str;
    char *new_str_pos = new_str;
    while ((pos = strstr(pos, _find))) {
        int bytes_to_copy = pos - last_pos;
        memcpy(new_str_pos, last_pos, bytes_to_copy);
        new_str_pos += bytes_to_copy;

        memcpy(new_str_pos, _replace, replace_len);
        new_str_pos += replace_len;

        pos += find_len;
        last_pos = pos;
    }

    strcpy(new_str_pos, last_pos);
    return new_str;
}

void su_strarr_append(char **_str_arr, const char *_append_str) {
    size_t current_length = strlen(*_str_arr);
    size_t append_length = strlen(_append_str);
    size_t current_size = current_length + 1;
    size_t required_size = current_length + append_length + 1;
    if (required_size > current_size) {
        size_t new_size = current_size;
        while (new_size < current_size + append_length + 1) {
            new_size *= 2;
        }
        char* new_str = realloc(*_str_arr, sizeof(char) * new_size);
        if (!new_str) {
            return;
        }
        *_str_arr = new_str;
    }
    strcpy(*_str_arr + current_length, _append_str);
}

bool su_str_2_file(const char* _path, const char* _str) {
    FILE *filePtr = fopen(_path, "w");
    if (filePtr == NULL) {
        return false;
    }
    fprintf(filePtr, "%s\n", _str);
    fclose(filePtr);
    return true;
}

bool su_mem_2_file(const char* _path, const char* _data, size_t _len) {
    FILE *filePtr = fopen(_path, "w");
    if (filePtr == NULL) {
        return false;
    }
    size_t written = fwrite(_data, sizeof(char), _len, filePtr);
    if (written < _len) {
        fclose(filePtr);
        return false;
    }
    fclose(filePtr);
    return true;
}

bool su_sizet_2_str(size_t _src, char* _dest) {
    if (sprintf(_dest, "%lu", _src) < 0) {
        return false;
    }
    return true;
}

bool su_uint32_2_str(uint32_t _src, char* _dest) {
    if (sprintf(_dest, "%u", _src) < 0) {
        return false;
    }
    return true;
}

bool str_2_uint32t(uint32_t _src, char* _dest) {
    if (sprintf(_dest, "%u", _src) < 0) {
        return false;
    }
    return true;
}

bool su_int32_2_str(int32_t _src, char* _dest) {
    if (sprintf(_dest, "%i", _src) < 0) {
        return false;
    }
    return true;
}


bool su_uint48_2_str(const uint8_t* _src, char* _dest) {
    sprintf(_dest, "%02X:%02X:%02X:%02X:%02X:%02X",
            _src[0], _src[1], _src[2], _src[3], _src[4], _src[5]);
    return true;
}

bool su_str_2_uint32(const char* _src, uint32_t* _dest) {
    char *end;
    errno = 0;
    size_t num = strtoul(_src, &end, 10);
    if (_src == end) {
        return false;
    }
    if (errno == ERANGE && num == ULONG_MAX) {
        return false;
    }
    if (errno != 0) {
        return false;
    }
    if (num > UINT32_MAX) {
        return false;
    }
    *_dest = (uint32_t) num;
    return true;
}

bool su_str_2_sizet(const char* _str, size_t *_dest) {
    if (!_str || !_dest) {
        return false;
    }
    char* end_ptr;
    errno = 0;
    unsigned long long result = strtoull(_str, &end_ptr, 10);
    if (end_ptr == _str || errno == ERANGE || result > SIZE_MAX) {
        return false;
    }
    *_dest = (size_t)result;
    return true;
}

bool su_str_2_int32(const char* _str, int32_t* _dest) {
    char *end;
    errno = 0;
    long num = strtol(_str, &end, 10);
    if ((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))
    || (errno != 0 && num == 0)) {
        return false;
    }
    if (end == _str) {
        return false;
    }
    if (num < INT32_MIN || num > INT32_MAX) {
        return false;
    }

    *_dest = (int32_t)num;
    return true;
}

// Takes a big-endian variable length string of digits and converts
// it to a uint8_t[16] arr
bool su_str_2_uint128(const char* _src, uint8_t* _dest) {
    memset(_dest, 0, 16);
    const char* ptr = _src;
    if (*ptr == '\0') {
        return false;
    }
    while (*ptr) {
        if (!isdigit((unsigned char)*ptr)) {
            return false;
        }

        uint8_t digit = *ptr - '0';
        int carry = 0;
        for (int i = 15; i >= 0; i--) {
            uint16_t temp = (uint16_t)_dest[i] * 10 + carry;
            _dest[i] = (uint8_t)(temp & 0xFF);
            carry = (temp >> 8) & 0xFF;
        }
        carry = digit;
        for (int i = 15; i >= 0; i--) {
            uint16_t temp = (uint16_t)_dest[i] + carry;
            _dest[i] = (uint8_t)(temp & 0xFF);
            carry = (temp >> 8) & 0xFF;
            if (carry == 0) {
                break;
            }
        }
        if (carry != 0) {
            return false;
        }
        ptr++;
    }
    return true;
}

bool su_str_hex_2_uint128(const char* _src, uint8_t* _dest) {
    if (strlen(_src) != 32) {
        return false;
    }

    for (int i = 0; i < 16; ++i) {
        char high_char = _src[2 * i];
        char low_char = _src[2 * i + 1];
        int high = 0;
        int low = 0;

        if ('0' <= high_char && high_char <= '9') {
            high = high_char - '0';
        } else if ('a' <= high_char && high_char <= 'f') {
            high = 10 + (high_char - 'a');
        } else if ('A' <= high_char && high_char <= 'F') {
            high = 10 + (high_char - 'A');
        } else {
            return false;
        }

        if ('0' <= low_char && low_char <= '9') {
            low = low_char - '0';
        } else if ('a' <= low_char && low_char <= 'f') {
            low = 10 + (low_char - 'a');
        } else if ('A' <= low_char && low_char <= 'F') {
            low = 10 + (low_char - 'A');
        } else {
            return false;
        }

        _dest[i] = (uint8_t)((high << 4) | low);
    }

    return true;
}

size_t SIZE_T_MAX_DIGITS() {
    return (int)floor(log10((double)SIZE_MAX)) + 1;
}

int su_strncnts(const char *str, char c, size_t n) {
    return memchr(str, (int)c, n) != NULL;
}

bool su_str_has_null(const char *buffer, size_t size) {
    return memchr(buffer, '\0', size) != NULL;
}

bool su_str_numeric(const char *str) {
    if (*str == '\0') return false;
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    return true;
}

bool su_uint128_2_ipv6_str(const uint8_t* _src, char* _dest) {
   int written = snprintf(
        _dest,
        40,
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
        _src[0], _src[1],
        _src[2], _src[3],
        _src[4], _src[5],
        _src[6], _src[7],
        _src[8], _src[9],
        _src[10], _src[11],
        _src[12], _src[13],
        _src[14], _src[15]
    );
    if (written != 39) {
        return false;
    }
    _dest[39] = '\0';

    return true;
}


static bool is_zero(const uint8_t* bytes) {
    for (int i = 0; i < 16; i++) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

// Function to convert a 128-bit unsigned integer to a decimal string
bool su_uint128_2_str(const uint8_t* _src, char* _dest) {
    uint8_t temp[16];
    memcpy(temp, _src, 16);

    char digits[40];
    int digit_count = 0;

    if (is_zero(temp)) {
        _dest[0] = '0';
        _dest[1] = '\0';
        return true;
    }

    while (!is_zero(temp)) {
        uint8_t carry = 0;
        for (int i = 0; i < 16; i++) {
            uint16_t current = ((uint16_t)carry << 8) | temp[i];
            temp[i] = current / 10;
            carry = current % 10;
        }
        if (digit_count >= 39) {
            return false;
        }
        digits[digit_count++] = '0' + carry;
    }
    for (int i = 0; i < digit_count; i++) {
        _dest[i] = digits[digit_count - i - 1];
    }
    _dest[digit_count] = '\0';

    return true;
}

const char* su_file_2_mem(const char* _path, size_t* _len) {
    FILE* file = fopen(_path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size == -1L) {
        fclose(file);
        return NULL;
    }


    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    * _len = bytes_read;
    return buffer;
}