#include <string.h>
#include <stdio.h>

#define EOK 0
#define memset_s(dest, destsz, value, count) (memset((dest), (value), (count)), 0)
#define memcpy_s(dest, destsz, src, count) (memcpy((dest), (src), (count)), 0)
#define memmove_s(dest, destsz, src, count) (memmove((dest), (src), (count)), 0)
#define strncpy_s(dest, destsz, src, count) (strncpy((dest), (src), (count)), 0)
#define strcpy_s(dest, destsz, src) (strcpy((dest), (src)), 0)
#define sprintf_s(dest, destsz, fmt, ...) (sprintf((dest), (fmt), __VA_ARGS__), 0)
#define vsprintf_s(dest, destsz, fmt, args) (vsprintf((dest), (fmt), (args)), 1)