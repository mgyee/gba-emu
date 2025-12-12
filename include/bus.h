#pragma once
#include "common.h"
#include "keypad.h"
#include "ppu.h"

typedef enum { ACCESS_NONSEQ = 0, ACCESS_SEQ = 1, ACCESS_CODE = 2 } Access;

typedef struct {
  u8 bios[0x4000];  // 16KB BIOS
  u8 wram[0x40000]; // 256KB Work RAM
  u8 iram[0x8000];  // 32KB Internal RAM
  // u8 io_regs[0x400]; // I/O Registers
  // u8 palette_ram[0x400]; // 1KB Palette RAM
  // u8 vram[0x20000];      // 128KB Video RAM
  // u8 oam[0x400];         // 1KB Object Attribute Memory
  u8 rom[0x2000000]; // Up to 32MB Game ROM
  // u8 sram[0x10000];      // Up to 64KB Save RAM

  PPU *ppu;
  Keypad *keypad;

  Access last_access;
  int cycle_count;
} Bus;

void bus_init(Bus *bus, PPU *ppu, Keypad *keypad);
u8 bus_read8(Bus *bus, u32 address, Access access);
void bus_write8(Bus *bus, u32 address, u8 value, Access access);
u16 bus_read16(Bus *bus, u32 address, Access access);
void bus_write16(Bus *bus, u32 address, u16 value, Access access);
u32 bus_read32(Bus *bus, u32 address, Access access);
void bus_write32(Bus *bus, u32 address, u32 value, Access access);

void bus_set_last_access(Bus *bus, Access access);

bool bus_load_rom(Bus *bus, const char *filename);
