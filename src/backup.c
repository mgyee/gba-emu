#include "backup.h"
#include "bus.h"
#include <string.h>

void backup_init(Backup *backup, u8 *rom_data, u32 rom_size) {
  if (memmem(rom_data, rom_size, "SRAM_", 5) != NULL) {
    backup->type = BACKUP_SRAM;
    backup->size = 32 * 1024;
  } else if (memmem(rom_data, rom_size, "FLASH_", 6) != NULL ||
             memmem(rom_data, rom_size, "FLASH512_", 9) != NULL) {
    backup->type = BACKUP_FLASH64;
    backup->size = 64 * 1024;
  } else if (memmem(rom_data, rom_size, "FLASH1M_", 8) != NULL) {
    backup->type = BACKUP_FLASH128;
    backup->size = 128 * 1024;
  }
  printf("%d\n", backup->type);

  backup->data = malloc(backup->size);
  memset(backup->data, 0xFF, backup->size);
}

u8 backup_read8(Backup *backup, u32 address) {
  u8 res;
  switch (backup->type) {
  case BACKUP_SRAM:
    res = read_mem8(backup->data, address & (backup->size - 1));
    break;
  case BACKUP_FLASH64:
    if (address == 0x0E000000) {
      res = 0x32;
    } else if (address == 0x0E000001) {
      res = 0x1B;
    } else {
      res = 0xFF;
    }
    break;
  case BACKUP_FLASH128:
    if (address == 0x0E000000) {
      res = 0x62;
    } else if (address == 0x0E000001) {
      res = 0x13;
    } else {
      res = 0xFF;
    }

    break;
  default:
    res = 0xFF;
    break;
  }
  return res;
}

void backup_write8(Backup *backup, u32 address, u8 val) {
  if (backup->type == BACKUP_SRAM) {
    write_mem8(backup->data, address & (backup->size - 1), val);
  }
}

u16 backup_read16(Backup *backup, u32 address) {
  u16 res;
  switch (backup->type) {
  case BACKUP_SRAM:
    res = read_mem8(backup->data, address & (backup->size - 1));
    res |= res << 8;
    break;
  default:
    res = 0xFFFF;
    break;
  }
  return res;
}

void backup_write16(Backup *backup, u32 address, u16 val) {
  if (backup->type == BACKUP_SRAM) {
    write_mem8(backup->data, address & (backup->size - 1), val);
    write_mem8(backup->data, (address + 1) & (backup->size - 1), val);
  }
}

u32 backup_read32(Backup *backup, u32 address) {
  u32 res;
  switch (backup->type) {
  case BACKUP_SRAM:
    res = read_mem8(backup->data, address & (backup->size - 1));
    res |= res << 24 | res << 16 | res << 8;
    break;
  default:
    res = 0xFFFF;
    break;
  }
  return res;
}

void backup_write32(Backup *backup, u32 address, u32 val) {
  if (backup->type == BACKUP_SRAM) {
    write_mem8(backup->data, address & (backup->size - 1), val);
    write_mem8(backup->data, (address + 1) & (backup->size - 1), val);
    write_mem8(backup->data, (address + 2) & (backup->size - 1), val);
    write_mem8(backup->data, (address + 3) & (backup->size - 1), val);
  }
}
