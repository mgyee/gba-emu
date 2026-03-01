#pragma once
#include "common.h"

typedef enum {
  BACKUP_NONE,
  BACKUP_SRAM,
  BACKUP_EEPROM,
  BACKUP_FLASH
} BackupType;

typedef struct {
  BackupType type;
  u8 *data;
  u32 size;
} Backup;

void backup_init(Backup *backup, BackupType type);
u8 backup_read8(Backup *backup, u32 address);
u16 backup_read16(Backup *backup, u32 address);
u32 backup_read32(Backup *backup, u32 address);

void backup_write8(Backup *backup, u32 address, u8 val);
void backup_write16(Backup *backup, u32 address, u16 val);
void backup_write32(Backup *backup, u32 address, u32 val);
