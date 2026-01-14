#include "common.h"

#define ROM_MAX_SIZE 0x2000000 // 32 MB

typedef struct {
  u8 *data;
  u32 size;
  char *title;
  char *code;
  char *maker;
} Rom;

bool load_rom(Rom *rom, const char *filename);
