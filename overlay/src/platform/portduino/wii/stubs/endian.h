// <endian.h> for devkitPPC (newlib has none): the glibc names the Crypto
// library expects under HOST_BUILD, from GCC's byte-order macros.
#pragma once
#include <stdint.h>
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BYTE_ORDER __BIG_ENDIAN
#define htole16(x) __builtin_bswap16(x)
#define le16toh(x) __builtin_bswap16(x)
#define htobe16(x) ((uint16_t)(x))
#define be16toh(x) ((uint16_t)(x))
#define htole32(x) __builtin_bswap32(x)
#define le32toh(x) __builtin_bswap32(x)
#define htobe32(x) ((uint32_t)(x))
#define be32toh(x) ((uint32_t)(x))
#define htole64(x) __builtin_bswap64(x)
#define le64toh(x) __builtin_bswap64(x)
#define htobe64(x) ((uint64_t)(x))
#define be64toh(x) ((uint64_t)(x))
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#define htole16(x) ((uint16_t)(x))
#define le16toh(x) ((uint16_t)(x))
#define htobe16(x) __builtin_bswap16(x)
#define be16toh(x) __builtin_bswap16(x)
#define htole32(x) ((uint32_t)(x))
#define le32toh(x) ((uint32_t)(x))
#define htobe32(x) __builtin_bswap32(x)
#define be32toh(x) __builtin_bswap32(x)
#define htole64(x) ((uint64_t)(x))
#define le64toh(x) ((uint64_t)(x))
#define htobe64(x) __builtin_bswap64(x)
#define be64toh(x) __builtin_bswap64(x)
#endif
