#include "backup.h"
#include "bus.h"
#include <string.h>

void backup_init(Backup *backup, BackupType type) {
  backup->type = type;
  switch (type) {
  case BACKUP_SRAM:
    backup->size = 32 * 1024;
    backup->data = malloc(backup->size);
    memset(backup->data, 0xFF, backup->size);
    break;
  default:
    break;
  }
}

u8 backup_read8(Backup *backup, u32 address) {
  if (backup->type == BACKUP_SRAM && backup->data) {
    return read_mem8(backup->data, address & (backup->size - 1));
  }
  return 0xFF;
}

void backup_write8(Backup *backup, u32 address, u8 val) {
  if (backup->type == BACKUP_SRAM && backup->data) {
    write_mem8(backup->data, address & (backup->size - 1), val);
  }
}

u16 backup_read16(Backup *backup, u32 address) {
  if (backup->type == BACKUP_SRAM && backup->data) {
    u16 res = read_mem8(backup->data, address & (backup->size - 1));
    res |= res << 8;
    return res;
  }
  return 0xFFFF;
}

void backup_write16(Backup *backup, u32 address, u16 val) {
  if (backup->type == BACKUP_SRAM && backup->data) {
    write_mem8(backup->data, address & (backup->size - 1), val);
    write_mem8(backup->data, (address + 1) & (backup->size - 1), val);
  }
}

u32 backup_read32(Backup *backup, u32 address) {
  if (backup->type == BACKUP_SRAM && backup->data) {
    u32 res = read_mem8(backup->data, address & (backup->size - 1));
    res |= res << 24 | res << 16 | res << 8;
    return res;
  }
  return 0xFFFFFFFF;
}

void backup_write32(Backup *backup, u32 address, u32 val) {
  if (backup->type == BACKUP_SRAM && backup->data) {
    write_mem8(backup->data, address & (backup->size - 1), val);
    write_mem8(backup->data, (address + 1) & (backup->size - 1), val);
    write_mem8(backup->data, (address + 2) & (backup->size - 1), val);
    write_mem8(backup->data, (address + 3) & (backup->size - 1), val);
  }
}
