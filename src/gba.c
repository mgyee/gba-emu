#include "gba.h"
#include <string.h>

bool gba_init(Gba *gba, const char *bios_path, const char *rom_path) {
  memset(gba, 0, sizeof(Gba));

  if (!load_bios(gba->bios, bios_path)) {
    printf("Failed to load BIOS: %s\n", bios_path);
    return false;
  }
  if (!load_rom(&gba->rom, rom_path)) {
    printf("Failed to load ROM: %s\n", rom_path);
    return false;
  }

  cpu_init(&gba->cpu);
  bus_init(&gba->bus);
  ppu_init(&gba->ppu);
  apu_init(&gba->apu);
  io_init(&gba->io);
  keypad_init(&gba->keypad);
  dma_init(&gba->dma);
  timer_init(&gba->tmr_mgr);
  interrupt_init(&gba->int_mgr);

  arm_fetch(gba);
  return true;
}

void gba_free(Gba *gba) {
  free(gba->rom.data);
  free(gba->rom.title);
  free(gba->rom.code);
  free(gba->rom.maker);
}

bool load_bios(u8 *bios, const char *bios_path) {
  FILE *file = fopen(bios_path, "rb");
  if (!file) {
    return false;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);
  size_t read = fread(bios, 1, file_size, file);
  fclose(file);
  return read > 0;
}
