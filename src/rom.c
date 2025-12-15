#include "rom.h"
#include <string.h>

bool load_rom(Rom *rom, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return false;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);
  rom->data = malloc(ROM_MAX_SIZE);
  size_t read = fread(rom->data, 1, file_size, file);
  rom->title = strndup((char *)(rom->data + 0xA0), 12);
  rom->code = strndup((char *)(rom->data + 0xAC), 4);
  rom->maker = strndup((char *)(rom->data + 0xB0), 2);
  fclose(file);
  return read > 0;
}
