#include "bus.h"
#include "io.h"
#include <stdio.h>
#include <string.h>

static u16 read_mem16(const u8 *data, u32 offset) {
  return data[offset] | (data[offset + 1] << 8);
}

static u32 read_mem32(const u8 *data, u32 offset) {
  return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) |
         (data[offset + 3] << 24);
}

static void write_mem16(u8 *data, u32 offset, u16 value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
}

static void write_mem32(u8 *data, u32 offset, u32 value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
  data[offset + 2] = (value >> 16) & 0xFF;
  data[offset + 3] = (value >> 24) & 0xFF;
}

void bus_init(Bus *bus, PPU *ppu, Keypad *keypad) {
  memset(bus, 0, sizeof(Bus));
  bus->ppu = ppu;
  bus->keypad = keypad;
}

void bus_set_last_access(Bus *bus, Access access) { bus->last_access = access; }

u8 bus_read8(Bus *bus, u32 address, Access access) {
  u8 res;
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    res = 0;
    break;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    res = bus->wram[offset];
    break;
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    res = bus->iram[offset];
    break;
  case 0x04: // IO REGISTERS
    res = io_read8(bus, address);
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    res = bus->ppu->palram[offset];
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    res = bus->ppu->vram[offset];
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    res = bus->ppu->oam[offset];
    break;
  case 0x08: // ROM
  case 0x09:
  case 0x0A: // Wait State 1
  case 0x0B:
  case 0x0C: // Wait State 2
  case 0x0D:
    offset = address & 0x1FFFFFF;
    res = bus->rom[offset];
    break;
  default:
    res = 0;
    break;
  }
  bus->cycle_count++;
  bus->last_access = ACCESS_SEQ;
  return res;
}

u16 bus_read16(Bus *bus, u32 address, Access access) {
  if ((access & ACCESS_CODE) == ACCESS_CODE) {
    address &= ~0x1;
  }
  u16 res;
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    res = 0;
    break;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    res = read_mem16(bus->wram, offset);
    break;
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    res = read_mem16(bus->iram, offset);
    break;
  case 0x04: // IO REGISTERS
    res = io_read16(bus, address);
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    res = read_mem16(bus->ppu->palram, offset);
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    res = read_mem16(bus->ppu->vram, offset);
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    res = read_mem16(bus->ppu->oam, offset);
    break;
  case 0x08: // ROM
  case 0x09:
  case 0x0A: // Wait State 1
  case 0x0B:
  case 0x0C: // Wait State 2
  case 0x0D:
    offset = address & 0x1FFFFFF;
    res = read_mem16(bus->rom, offset);
    break;
  default:
    res = 0;
    break;
  }
  bus->cycle_count++;
  bus->last_access = ACCESS_SEQ;
  return res;
}

u32 bus_read32(Bus *bus, u32 address, Access access) {
  if ((access & ACCESS_CODE) == ACCESS_CODE) {
    address &= ~0x3;
  }
  u32 res;
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    res = 0;
    break;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    res = read_mem32(bus->wram, offset);
    break;
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    res = read_mem32(bus->iram, offset);
    break;
  case 0x04: // IO REGISTERS
    res = io_read32(bus, address);
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    res = read_mem32(bus->ppu->palram, offset);
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    res = read_mem32(bus->ppu->vram, offset);
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    res = read_mem32(bus->ppu->oam, offset);
    break;
  case 0x08: // ROM
  case 0x09:
  case 0x0A: // Wait State 1
  case 0x0B:
  case 0x0C: // Wait State 2
  case 0x0D:
    offset = address & 0x1FFFFFF;
    res = read_mem32(bus->rom, offset);
    break;
  default:
    res = 0;
    break;
  }
  bus->cycle_count++;
  bus->last_access = ACCESS_SEQ;
  return res;
}

void bus_write8(Bus *bus, u32 address, u8 data, Access access) {
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    break;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    bus->wram[offset] = data;
    break;
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    bus->iram[offset] = data;
    break;
  case 0x04: // IO REGISTERS
    io_write8(bus, address, data);
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    bus->ppu->palram[offset] = data;
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    bus->ppu->vram[offset] = data;
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    bus->ppu->oam[offset] = data;
    break;
  case 0x08: // ROM
    break;
  default:
    break;
  }
  bus->cycle_count++;
  bus->last_access = ACCESS_SEQ;
}
void bus_write16(Bus *bus, u32 address, u16 data, Access access) {
  if ((access & ACCESS_CODE) == ACCESS_CODE) {
    address &= ~0x1;
  }
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    break;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    write_mem16(bus->wram, offset, data);
    break;
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    write_mem16(bus->iram, offset, data);
    break;
  case 0x04: // IO REGISTERS
    io_write16(bus, address, data);
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    write_mem16(bus->ppu->palram, offset, data);
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    write_mem16(bus->ppu->vram, offset, data);
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    write_mem16(bus->ppu->oam, offset, data);
    break;
  case 0x08: // ROM
    break;
  default:
    break;
  }
  bus->cycle_count++;
  bus->last_access = ACCESS_SEQ;
}

void bus_write32(Bus *bus, u32 address, u32 data, Access access) {
  if ((access & ACCESS_CODE) == ACCESS_CODE) {
    address &= ~0x3;
  }
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    break;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    write_mem32(bus->wram, offset, data);
    break;
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    write_mem32(bus->iram, offset, data);
    break;
  case 0x04: // IO REGISTERS
    io_write32(bus, address, data);
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    write_mem32(bus->ppu->palram, offset, data);
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    write_mem32(bus->ppu->vram, offset, data);
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    write_mem32(bus->ppu->oam, offset, data);
    break;
  case 0x08: // ROM
    break;
  default:
    break;
  }
  bus->cycle_count++;
  bus->last_access = ACCESS_SEQ;
}

bool bus_load_rom(Bus *bus, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return false;
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  size_t read = fread(bus->rom, 1, file_size, file);
  fclose(file);
  return read > 0;
}
