#include "bus.h"
#include <stdio.h>
#include <string.h>

void bus_init(Bus *bus) { memset(bus, 0, sizeof(Bus)); }

u8 bus_read8(Bus *bus, u32 address) {
  u32 offset;
  switch (address >> 24) {
  case 0x00: // BIOS
    return 0;
  case 0x02: // WRAM
    offset = address & 0x3FFFF;
    return bus->wram[offset];
  case 0x03: // IRAM
    offset = address & 0x7FFF;
    return bus->iram[offset];
  case 0x04: // IO REGISTERS
             // TODO
    return 0;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    return bus->palette_ram[offset];
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    return bus->vram[offset];
  case 0x07: // OAM
    offset = address & 0x3FF;
    return bus->oam[offset];
  case 0x08: // ROM
  case 0x09:
  case 0x0A: // Wait State 1
  case 0x0B:
  case 0x0C: // Wait State 2
  case 0x0D:
    offset = address & 0x1FFFFFF;
    return bus->rom[offset];
  default:
    return 0;
  }
}

void bus_write8(Bus *bus, u32 address, u8 data) {
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
             // TODO
    break;
  case 0x05: // PALETTE RAM
    offset = address & 0x3FF;
    bus->palette_ram[offset] = data;
    break;
  case 0x06: // VRAM
    offset = address & 0x1FFFF;
    if (offset >= 0x18000) {
      offset &= 0x17FFF;
    }
    bus->vram[offset] = data;
    break;
  case 0x07: // OAM
    offset = address & 0x3FF;
    bus->oam[offset] = data;
    break;
  case 0x08: // ROM
    break;
  default:
    break;
  }
}

u16 bus_read16(Bus *bus, u32 address) {
  u8 low = bus_read8(bus, address);
  u8 high = bus_read8(bus, address + 1);
  return (high << 8) | low;
}

u32 bus_read32(Bus *bus, u32 address) {
  u16 low = bus_read16(bus, address);
  u16 high = bus_read16(bus, address + 2);
  return (high << 16) | low;
}

void bus_write16(Bus *bus, u32 address, u16 data) {
  u8 low = data & 0xFF;
  u8 high = (data >> 8) & 0xFF;
  bus_write8(bus, address, low);
  bus_write8(bus, address + 1, high);
}

void bus_write32(Bus *bus, u32 address, u32 data) {
  u16 low = data & 0xFFFF;
  u16 high = (data >> 16) & 0xFFFF;
  bus_write16(bus, address, low);
  bus_write16(bus, address + 2, high);
}

bool bus_load_rom(Bus *bus, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return false;
  }
  size_t read = fread(bus->rom, 1, sizeof(bus->rom), file);
  fclose(file);
  return read > 0;
}
