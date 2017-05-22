#ifndef _N64_TYPES_H_
#define _N64_TYPES_H_
#include <stdint.h>

#define BIT(n) (1 << (n))

//RAM address to/from uncached
//use only 0x80xxxxxx / 0xA0xxxxxx
#define CACHED(addr) (((u32)(addr)) | 0x20000000)
#define UNCACHED(addr) (((u32)(addr)) & ~0x20000000)

typedef u32 rspPtr;
typedef struct { float x, y, z; } vec3f;
typedef struct { s16   x, y, z; } vec3i;

#endif //_N64_TYPES_H_
