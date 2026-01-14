#include "bus.h"
#include "cpu.h"
#include "gba.h"
#include "io.h"
#include <stdio.h>
#include <string.h>

static inline u8 read_mem8(const u8 *data, u32 offset) { return data[offset]; }

static inline u16 read_mem16(const u8 *data, u32 offset) {
  return *(u16 *)(data + offset);
}

static inline u32 read_mem32(const u8 *data, u32 offset) {
  return *(u32 *)(data + offset);
}

static inline void write_mem8(u8 *data, u32 offset, u8 value) {
  data[offset] = value;
}

static inline void write_mem16(u8 *data, u32 offset, u16 value) {
  *(u16 *)(data + offset) = value;
}

static inline void write_mem32(u8 *data, u32 offset, u32 value) {
  *(u32 *)(data + offset) = value;
}

void bus_init_waitstates(Bus *bus) {
  for (int access = 0; access < 2; access++) {
    for (int region = 0; region < 16; region++) {
      bus->wait_16[access][region] = 1;
      bus->wait_32[access][region] = 1;
    }
  }

  bus->wait_16[ACCESS_SEQ][REGION_BIOS] = 1;
  bus->wait_16[ACCESS_NONSEQ][REGION_BIOS] = 1;
  bus->wait_32[ACCESS_SEQ][REGION_BIOS] = 1;
  bus->wait_32[ACCESS_NONSEQ][REGION_BIOS] = 1;

  bus->wait_16[ACCESS_SEQ][REGION_EWRAM] = 3;
  bus->wait_16[ACCESS_NONSEQ][REGION_EWRAM] = 3;
  bus->wait_32[ACCESS_SEQ][REGION_EWRAM] = 6;
  bus->wait_32[ACCESS_NONSEQ][REGION_EWRAM] = 6;

  bus->wait_16[ACCESS_SEQ][REGION_IWRAM] = 1;
  bus->wait_16[ACCESS_NONSEQ][REGION_IWRAM] = 1;
  bus->wait_32[ACCESS_SEQ][REGION_IWRAM] = 1;
  bus->wait_32[ACCESS_NONSEQ][REGION_IWRAM] = 1;

  bus->wait_16[ACCESS_SEQ][REGION_IO] = 1;
  bus->wait_16[ACCESS_NONSEQ][REGION_IO] = 1;
  bus->wait_32[ACCESS_SEQ][REGION_IO] = 1;
  bus->wait_32[ACCESS_NONSEQ][REGION_IO] = 1;

  bus->wait_16[ACCESS_SEQ][REGION_PALETTE] = 1;
  bus->wait_16[ACCESS_NONSEQ][REGION_PALETTE] = 1;
  bus->wait_32[ACCESS_SEQ][REGION_PALETTE] = 2;
  bus->wait_32[ACCESS_NONSEQ][REGION_PALETTE] = 2;

  bus->wait_16[ACCESS_SEQ][REGION_VRAM] = 1;
  bus->wait_16[ACCESS_NONSEQ][REGION_VRAM] = 1;
  bus->wait_32[ACCESS_SEQ][REGION_VRAM] = 2;
  bus->wait_32[ACCESS_NONSEQ][REGION_VRAM] = 2;

  bus->wait_16[ACCESS_SEQ][REGION_OAM] = 1;
  bus->wait_16[ACCESS_NONSEQ][REGION_OAM] = 1;
  bus->wait_32[ACCESS_SEQ][REGION_OAM] = 1;
  bus->wait_32[ACCESS_NONSEQ][REGION_OAM] = 1;

  bus_update_waitstates(bus, 0);
}

void bus_update_waitstates(Bus *bus, u16 waitcnt) {
  static const int ws0_n[] = {4, 3, 2, 8};
  static const int ws0_s[] = {2, 1};
  static const int ws1_n[] = {4, 3, 2, 8};
  static const int ws1_s[] = {4, 1};
  static const int ws2_n[] = {4, 3, 2, 8};
  static const int ws2_s[] = {8, 1};
  static const int sram[] = {4, 3, 2, 8};

  int ws0_n_idx = GET_BITS(waitcnt, 2, 2);
  int ws0_s_idx = TEST_BIT(waitcnt, 4);
  int ws1_n_idx = GET_BITS(waitcnt, 5, 2);
  int ws1_s_idx = TEST_BIT(waitcnt, 7);
  int ws2_n_idx = GET_BITS(waitcnt, 8, 2);
  int ws2_s_idx = TEST_BIT(waitcnt, 10);

  int wait_n[] = {ws0_n[ws0_n_idx], ws1_n[ws1_n_idx], ws2_n[ws2_n_idx]};
  int wait_s[] = {ws0_s[ws0_s_idx], ws1_s[ws1_s_idx], ws2_s[ws2_s_idx]};

  for (int region = REGION_CART_WS0_A; region <= REGION_CART_WS2_B; region++) {
    int ws_idx = (region - REGION_CART_WS0_A) / 2;
    bus->wait_16[ACCESS_NONSEQ][region] = wait_n[ws_idx] + 1;
    bus->wait_16[ACCESS_SEQ][region] = wait_s[ws_idx] + 1;
    bus->wait_32[ACCESS_NONSEQ][region] =
        wait_n[ws_idx] + 1 + wait_s[ws_idx] + 1;
    bus->wait_32[ACCESS_SEQ][region] = wait_s[ws_idx] + 1 + wait_s[ws_idx] + 1;
  }

  int sram_idx = GET_BITS(waitcnt, 0, 2);

  for (int region = REGION_SRAM; region <= REGION_UNUSED; region++) {
    bus->wait_16[ACCESS_NONSEQ][region] = sram[sram_idx] + 1;
    bus->wait_16[ACCESS_SEQ][region] = sram[sram_idx] + 1;
    bus->wait_32[ACCESS_NONSEQ][region] = sram[sram_idx] + 1;
    bus->wait_32[ACCESS_SEQ][region] = sram[sram_idx] + 1;
  }
}

void bus_init(Bus *bus) {
  memset(bus, 0, sizeof(Bus));
  bus_init_waitstates(bus);
}

inline int get_region(u32 address) { return address >> 24; }

static u32 get_openbus_value(Gba *gba, u32 address) {
  Cpu *cpu = &gba->cpu;

  if (!(CPSR & CPSR_T)) {
    return cpu->pipeline[1];
  }

  u32 pc = PC;
  int region = get_region(address);
  u16 lsw, msw;

  if (region == REGION_EWRAM || region == REGION_PALETTE ||
      region == REGION_VRAM ||
      (region >= REGION_CART_WS0_A && region <= REGION_CART_WS2_B)) {
    u16 next = (u16)cpu->pipeline[1];
    lsw = next;
    msw = next;
  } else if (region == REGION_BIOS || region == REGION_OAM) {
    if ((pc & 2) == 0) {
      lsw = (u16)cpu->pipeline[1];
      msw = (u16)(cpu->pipeline[1] >> 16);
    } else {
      lsw = (u16)(cpu->pipeline[0] >> 16);
      msw = (u16)cpu->pipeline[1];
    }
  } else if (region == REGION_IWRAM) {
    if ((pc & 2) == 0) {
      lsw = (u16)cpu->pipeline[1];
      msw = (u16)cpu->pipeline[1] >> 16;
    } else {
      lsw = (u16)cpu->pipeline[1];
      msw = (u16)cpu->pipeline[1];
    }
  } else {
    u16 next = (u16)cpu->pipeline[1];
    lsw = next;
    msw = next;
  }

  return ((u32)msw << 16) | lsw;
}

static void add_cycles(Gba *gba, Access access, int region, int size) {
  Bus *bus = &gba->bus;
  int cycles = 0;
  region &= 0xF;
  if (size == 1 || size == 2) {
    cycles = bus->wait_16[access & 1][region];
  } else if (size == 4) {
    cycles = bus->wait_32[access & 1][region];
  }
  scheduler_step(&gba->scheduler, cycles);
}

u32 bus_read_bios(Gba *gba, u32 address) {
  u32 offset = address & ~3;
  if (PC < BIOS_SIZE) {
    gba->bus.bios_last_load = read_mem32(gba->bios, offset);
  }
  ShiftRes sh_res = barrel_shifter(
      &gba->cpu, SHIFT_ROR, gba->bus.bios_last_load, (address & 3) * 8, false);
  return sh_res.value;
}

u8 bus_read8(Gba *gba, u32 address, Access access) {
  u8 res;
  u32 offset;
  int region = get_region(address);
  switch (region) {
  case REGION_BIOS:
    if (address < BIOS_SIZE) {
      res = bus_read_bios(gba, address);
    } else {
      u32 openbus = get_openbus_value(gba, address);
      res = (openbus >> ((address & 3) * 8)) & 0xFF;
    }
    break;
  case REGION_EWRAM:
    offset = address & 0x3FFFF;
    res = read_mem8(gba->ewram, offset);
    break;
  case REGION_IWRAM:
    offset = address & 0x7FFF;
    res = read_mem8(gba->iwram, offset);
    break;
  case REGION_IO:
    res = io_read8(gba, address);
    break;
  case REGION_PALETTE:
    offset = address & 0x3FF;
    res = read_mem8(gba->ppu.palram, offset);
    break;
  case REGION_VRAM:
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    res = read_mem8(gba->ppu.vram, offset);
    break;
  case REGION_OAM:
    offset = address & 0x3FF;
    res = read_mem8(gba->ppu.oam, offset);
    break;
  case REGION_CART_WS0_A:
  case REGION_CART_WS0_B:
  case REGION_CART_WS1_A:
  case REGION_CART_WS1_B:
  case REGION_CART_WS2_A:
  case REGION_CART_WS2_B:
    offset = address & 0x1FFFFFF;
    if (offset < gba->rom.size) {
      res = read_mem8(gba->rom.data, offset);
    } else {
      res = ((address >> 1) >> ((address & 1) * 8)) & 0xFF;
    }
    if ((address & 0x1FFFF) == 0) {
      access = ACCESS_NONSEQ;
    }
    break;
  case REGION_SRAM:
    offset = address & 0xFFFF;
    if (offset < 0x10000) {
      res = 0xFF;
    } else {
      res = 0xFF;
    }
    break;
  default: {
    u32 openbus = get_openbus_value(gba, address);
    res = (openbus >> ((address & 3) * 8)) & 0xFF;
    break;
  }
  }
  add_cycles(gba, access, region, 1);
  return res;
}

u16 bus_read16(Gba *gba, u32 address, Access access) {
  u16 res;
  u32 offset;
  address &= ~1;
  int region = get_region(address);
  switch (region) {
  case REGION_BIOS:
    if (address < BIOS_SIZE) {
      res = bus_read_bios(gba, address);
    } else {
      u32 openbus = get_openbus_value(gba, address);
      res = (openbus >> ((address & 2) * 8)) & 0xFFFF;
    }
    break;
  case REGION_EWRAM:
    offset = address & 0x3FFFF;
    res = read_mem16(gba->ewram, offset);
    break;
  case REGION_IWRAM:
    offset = address & 0x7FFF;
    res = read_mem16(gba->iwram, offset);
    break;
  case REGION_IO:
    res = io_read16(gba, address);
    break;
  case REGION_PALETTE:
    offset = address & 0x3FF;
    res = read_mem16(gba->ppu.palram, offset);
    break;
  case REGION_VRAM:
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    res = read_mem16(gba->ppu.vram, offset);
    break;
  case REGION_OAM:
    offset = address & 0x3FF;
    res = read_mem16(gba->ppu.oam, offset);
    break;
  case REGION_CART_WS0_A:
  case REGION_CART_WS0_B:
  case REGION_CART_WS1_A:
  case REGION_CART_WS1_B:
  case REGION_CART_WS2_A:
  case REGION_CART_WS2_B:
    offset = address & 0x1FFFFFF;
    if (offset <= gba->rom.size) {
      res = read_mem16(gba->rom.data, offset);
    } else {
      res = (address >> 1) & 0xFFFF;
    }
    if ((address & 0x1FFFF) == 0) {
      access = ACCESS_NONSEQ;
    }
    break;
  case REGION_SRAM:
    offset = address & 0xFFFF;
    res = 0xFFFF;
    break;
  default: {
    u32 openbus = get_openbus_value(gba, address);
    res = (openbus >> ((address & 2) * 8)) & 0xFFFF;
    break;
  }
  }
  add_cycles(gba, access, region, 2);
  return res;
}

u32 bus_read32(Gba *gba, u32 address, Access access) {
  u32 res;
  u32 offset;
  address &= ~3;
  int region = get_region(address);
  switch (region) {
  case REGION_BIOS:
    if (address < BIOS_SIZE) {
      res = bus_read_bios(gba, address);
    } else {
      res = get_openbus_value(gba, address);
    }
    break;
  case REGION_EWRAM:
    offset = address & 0x3FFFF;
    res = read_mem32(gba->ewram, offset);
    break;
  case REGION_IWRAM:
    offset = address & 0x7FFF;
    res = read_mem32(gba->iwram, offset);
    break;
  case REGION_IO:
    res = io_read32(gba, address);
    break;
  case REGION_PALETTE:
    offset = address & 0x3FF;
    res = read_mem32(gba->ppu.palram, offset);
    break;
  case REGION_VRAM:
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    res = read_mem32(gba->ppu.vram, offset);
    break;
  case REGION_OAM:
    offset = address & 0x3FF;
    res = read_mem32(gba->ppu.oam, offset);
    break;
  case REGION_CART_WS0_A:
  case REGION_CART_WS0_B:
  case REGION_CART_WS1_A:
  case REGION_CART_WS1_B:
  case REGION_CART_WS2_A:
  case REGION_CART_WS2_B:
    offset = address & 0x1FFFFFF;
    if (offset <= gba->rom.size) {
      res = read_mem32(gba->rom.data, offset);
    } else {
      u16 lower = (address >> 1) & 0xFFFF;
      u16 upper = ((address >> 1) + 1) & 0xFFFF;
      res = ((u32)upper << 16) | lower;
    }
    if ((address & 0x1FFFF) == 0) {
      access = ACCESS_NONSEQ;
    }
    break;
  case REGION_SRAM:
    offset = address & 0xFFFF;
    res = 0xFFFFFFFF;
    break;
  default:
    res = get_openbus_value(gba, address);
    break;
  }
  add_cycles(gba, access, region, 4);
  return res;
}

void bus_write8(Gba *gba, u32 address, u8 data, Access access) {
  u32 offset;
  int region = get_region(address);
  switch (region) {
  case REGION_BIOS:
    break;
  case REGION_EWRAM:
    offset = address & 0x3FFFF;
    write_mem8(gba->ewram, offset, data);
    break;
  case REGION_IWRAM:
    offset = address & 0x7FFF;
    write_mem8(gba->iwram, offset, data);
    break;
  case REGION_IO:
    io_write8(gba, address, data);
    break;
  case REGION_PALETTE:
    offset = address & 0x3FF;
    write_mem16(gba->ppu.palram, offset, (data << 8) | data);
    break;
  case REGION_VRAM:
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    if (offset >= 0x10000) {
      // obj vram
      break;
    } else {
      write_mem16(gba->ppu.vram, offset, (data << 8) | data);
    }
    break;
  case REGION_OAM:
    return;
    break;
  default:
    break;
  }
  add_cycles(gba, access, region, 1);
}

void bus_write16(Gba *gba, u32 address, u16 data, Access access) {
  u32 offset;
  address &= ~1;
  int region = get_region(address);
  switch (region) {
  case REGION_BIOS:
    break;
  case REGION_EWRAM:
    offset = address & 0x3FFFF;
    write_mem16(gba->ewram, offset, data);
    break;
  case REGION_IWRAM:
    offset = address & 0x7FFF;
    write_mem16(gba->iwram, offset, data);
    break;
  case REGION_IO:
    io_write16(gba, address, data);
    break;
  case REGION_PALETTE:
    offset = address & 0x3FF;
    write_mem16(gba->ppu.palram, offset, data);
    break;
  case REGION_VRAM:
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    write_mem16(gba->ppu.vram, offset, data);
    break;
  case REGION_OAM:
    offset = address & 0x3FF;
    write_mem16(gba->ppu.oam, offset, data);
    break;
  default:
    break;
  }
  add_cycles(gba, access, region, 2);
}

void bus_write32(Gba *gba, u32 address, u32 data, Access access) {
  u32 offset;
  address &= ~3;
  int region = get_region(address);
  switch (region) {
  case REGION_BIOS:
    break;
  case REGION_EWRAM:
    offset = address & 0x3FFFF;
    write_mem32(gba->ewram, offset, data);
    break;
  case REGION_IWRAM:
    offset = address & 0x7FFF;
    write_mem32(gba->iwram, offset, data);
    break;
  case REGION_IO:
    io_write32(gba, address, data);
    break;
  case REGION_PALETTE:
    offset = address & 0x3FF;
    write_mem32(gba->ppu.palram, offset, data);
    break;
  case REGION_VRAM:
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    write_mem32(gba->ppu.vram, offset, data);
    break;
  case REGION_OAM:
    offset = address & 0x3FF;
    write_mem32(gba->ppu.oam, offset, data);
    break;
  default:
    break;
  }
  add_cycles(gba, access, region, 4);
}
