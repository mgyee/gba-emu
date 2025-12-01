#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define BIT(n) (1U << (n))
#define TEST_BIT(val, n) ((val) & BIT(n))
#define GET_BITS(val, pos, len) (((val) >> (pos)) & ((1U << (len)) - 1))

#define NOT_YET_IMPLEMENTED(str)                                               \
  printf("%s not yet implemented: %s:%d\n", str, __FILE__, __LINE__);          \
  exit(1);
