#pragma once
#include "apu.h"
#include "bus.h"
#include "common.h"
#include "cpu.h"
#include "dma.h"
#include "interrupt.h"
#include "io.h"
#include "keypad.h"
#include "ppu.h"
#include "rom.h"
#include "scheduler.h"
#include "timer.h"

struct Gba {
  u8 bios[0x4000];

  Rom rom;

  Cpu cpu;

  Bus bus;

  Scheduler scheduler;

  Io io;

  Ppu ppu;

  Apu apu;

  InterruptManager int_mgr;

  Dma dma;

  TimerManager tmr_mgr;

  // cartrige
  //
  u8 ewram[0x40000];
  u8 iwram[0x8000];

  Keypad keypad;
};

bool gba_init(Gba *gba, const char *bios_path, const char *rom_path);

void gba_free(Gba *gba);

bool load_bios(u8 *bios, const char *bios_path);
