#include "bus.h"
#include "common.h"

// Display Control
#define DISPCNT 0x04000000
#define GREENSWAP 0x04000002
#define DISPSTAT 0x04000004
#define VCOUNT 0x04000006

// Background Control
#define BG0CNT 0x04000008
#define BG1CNT 0x0400000A
#define BG2CNT 0x0400000C
#define BG3CNT 0x0400000E

// Background Scrolling
#define BG0HOFS 0x04000010
#define BG0VOFS 0x04000012
#define BG1HOFS 0x04000014
#define BG1VOFS 0x04000016
#define BG2HOFS 0x04000018
#define BG2VOFS 0x0400001A
#define BG3HOFS 0x0400001C
#define BG3VOFS 0x0400001E

// Background Affine Parameters
#define BG2PA 0x04000020
#define BG2PB 0x04000022
#define BG2PC 0x04000024
#define BG2PD 0x04000026
#define BG2X 0x04000028
#define BG2Y 0x0400002C
#define BG3PA 0x04000030
#define BG3PB 0x04000032
#define BG3PC 0x04000034
#define BG3PD 0x04000036
#define BG3X 0x04000038
#define BG3Y 0x0400003C

// Windowing
#define WIN0H 0x04000040
#define WIN1H 0x04000042
#define WIN0V 0x04000044
#define WIN1V 0x04000046
#define WININ 0x04000048
#define WINOUT 0x0400004A

// Special Effects
#define MOSAIC 0x0400004C
#define BLDCNT 0x04000050
#define BLDALPHA 0x04000052
#define BLDY 0x04000054

// Key Input
#define KEYINPUT 0x04000130
#define KEYCNT 0x04000132

u8 io_read8(Bus *bus, u32 addr);
void io_write8(Bus *bus, u32 addr, u8 val);
u16 io_read16(Bus *bus, u32 addr);
void io_write16(Bus *bus, u32 addr, u16 val);
u32 io_read32(Bus *bus, u32 addr);
void io_write32(Bus *bus, u32 addr, u32 val);
