#ifndef STRING_UTIL_H_
#define STRING_UTIL_H_

#include <stddef.h>
#include <stdint.h>

#define NEGATIVE_CHAR 1
#define UINT32_MAX_DIGITS 10
#define INT32_MAX_DIGITS (10 + NEGATIVE_CHAR)

const char** su_str_split(const char* _str, const char _delim, size_t* _arr_len);

void su_free_split(const char** _strs, size_t _num_strs);

void su_trim_line(char* _line);

void su_to_lower(char* _str);

void su_to_upper(char* _str);

char* su_find_replace(const char* _str, const char* _find, const char* _replace);

void su_strarr_append(char** _str_arr, const char* _append_str);

bool su_str_2_file(const char* _path, const char* _str);

bool su_mem_2_file(const char* _path, const char* _data, size_t _len);

bool su_sizet_2_str(size_t _src, char* _dest);

bool su_int32_2_str(int32_t _src, char* _dest);

bool su_uint32_2_str(uint32_t _src, char* _dest);

bool su_uint48_2_str(const uint8_t* _src, char* _dest);

bool su_uint128_2_str(const uint8_t* _src, char* _dest);

bool su_str_2_int32(const char* _str, int32_t* _dest);

bool su_str_2_uint32(const char* _src, uint32_t* _dest);

bool su_str_2_uint128(const char* _src, uint8_t* _dest);

bool su_str_hex_2_uint128(const char* _src, uint8_t* _dest);

bool su_str_2_sizet(const char* _str, size_t* _dest);

size_t SIZE_T_MAX_DIGITS();

int su_strncnts(const char *str, char c, size_t n);

bool su_str_has_null(const char *buffer, size_t size);

bool su_str_numeric(const char *str);

bool su_uint128_2_ipv6_str(const uint8_t* _src, char* _dest);

const char* su_file_2_mem(const char* _path, size_t* _len);

#endif // STRING_UTIL_H_
