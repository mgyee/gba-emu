#pragma once
#include "common.h"

typedef enum { ACCESS_NONSEQ = 0, ACCESS_SEQ = 1 } Access;

#define REGION_BIOS 0x0
#define REGION_EWRAM 0x2
#define REGION_IWRAM 0x3
#define REGION_IO 0x4
#define REGION_PALETTE 0x5
#define REGION_VRAM 0x6
#define REGION_OAM 0x7
#define REGION_CART_WS0_A 0x8
#define REGION_CART_WS0_B 0x9
#define REGION_CART_WS1_A 0xA
#define REGION_CART_WS1_B 0xB
#define REGION_CART_WS2_A 0xC
#define REGION_CART_WS2_B 0xD
#define REGION_SRAM 0xE
#define REGION_UNUSED 0xF

struct Bus {
  int wait_16[2][16];
  int wait_32[2][16];

  int cycle_count;
};

void bus_init(Bus *bus);
u8 bus_read8(Gba *gba, u32 address, Access access);
void bus_write8(Gba *gba, u32 address, u8 value, Access access);
u16 bus_read16(Gba *gba, u32 address, Access access);
void bus_write16(Gba *gba, u32 address, u16 value, Access access);
u32 bus_read32(Gba *gba, u32 address, Access access);
void bus_write32(Gba *gba, u32 address, u32 value, Access access);

void bus_init_waitstates(Bus *bus);
void bus_update_waitstates(Bus *bus, u16 waitcnt);
