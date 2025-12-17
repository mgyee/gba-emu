#pragma once
#include "bus.h"
#include "common.h"
#include "cpu.h"
#include "interrupt.h"
#include "io.h"
#include "keypad.h"
#include "ppu.h"
#include "rom.h"

struct Gba {
  u8 bios[0x4000];

  Rom rom;

  Cpu cpu;

  Bus bus;

  // scheduler

  Io io;

  Ppu ppu;

  // apu

  InterruptManager int_mgr;

  // dma

  // timer

  // cartrige
  //
  u8 ewram[0x40000];
  u8 iwram[0x8000];

  Keypad keypad;
};

bool gba_init(Gba *gba, const char *bios_path, const char *rom_path);

void gba_free(Gba *gba);

bool load_bios(u8 *bios, const char *bios_path);
