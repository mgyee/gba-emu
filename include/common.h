#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef unsigned int uint;

typedef struct Gba Gba;
typedef struct Bus Bus;
typedef struct Cpu Cpu;
typedef struct Ppu Ppu;
typedef struct Keypad Keypad;

#define SCALE 4
#define SCREEN_WIDTH 240 * SCALE
#define SCREEN_HEIGHT 160 * SCALE

#define CYCLES_PER_FRAME 280896
#define FRAME_TIME_MS (1000.0 / 59.73)

#define BIT(n) (1U << (n))
#define TEST_BIT(val, n) ((val) & BIT(n))
#define GET_BITS(val, pos, len) (((val) >> (pos)) & ((1U << (len)) - 1))

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#define NOT_YET_IMPLEMENTED(str)                                               \
  printf("%s not yet implemented: %s:%d\n", str, __FILE__, __LINE__);          \
  exit(1);
