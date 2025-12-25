#pragma once
#include "common.h"

#define PIXELS_WIDTH 240
#define PIXELS_HEIGHT 160

#define TRANSPARENT 0x8000
#define WHITE 0x7FFF
#define BLACK 0x0000

#define OBJ_IDX 4
#define BACKDROP_IDX 5

typedef enum {
  BLEND_NONE,
  BLEND_ALPHA,
  BLEND_BRIGHTEN,
  BLEND_DARKEN,
} Effect;

struct Ppu {
  u32 framebuffer[PIXELS_WIDTH * PIXELS_HEIGHT];
  int cycle;

  u8 palram[0x400];
  u8 vram[0x18000];
  u8 oam[0x400];

  struct {
    struct {
      u16 val;
      int mode;
      int cg_mode;
      int frame;
      int hblank_oam_access;
      int oam_mapping_1d;
      int forced_blank;
      int enable[8];
    } dispcnt;
    u16 greenswap;
    struct {
      u16 val;
      int vblank;
      int hblank;
      int vcounter;
      int vblank_irq;
      int hblank_irq;
      int vcounter_irq;
      int vcount_setting;
    } dispstat;
    u16 vcount;
    struct {
      u16 val;
      int priority;
      int char_base_block;
      int mosaic;
      int colors;
      int screen_base_block;
      int display_area_overflow;
      int screen_size;
    } bgcnt[4];
    u16 bghofs[4];
    u16 bgvofs[4];
    s16 bgpa[2];
    s16 bgpb[2];
    s16 bgpc[2];
    s16 bgpd[2];
    struct {
      s32 current;
      s32 internal;
    } bgx[2], bgy[2];

    u16 winh[2];
    u16 winv[2];

    struct {
      u16 val;
      int enable[2][6];
    } winin, winout;

    struct {
      u16 val;
      int bg_h;
      int bg_v;
      int obj_h;
      int obj_v;
    } mosaic;

    struct {
      u16 val;
      Effect effect;
      int targets[2][6];
      int color_special_effect;
    } blendcnt;

    int eva;
    int evb;
    int evy;
  } LCD;
};

typedef enum {
  GFXMODE_NORMAL,
  GFXMODE_BLEND,
  GFXMODE_WINDOW,
  GFXMODE_FORBIDDEN
} GraphicsMode;

typedef struct {
  u16 attr[3];
  u16 dummy;
} ObjAttr;

typedef struct {
  u16 fill0[3];
  s16 pa;
  u16 fill1[3];
  s16 pb;
  u16 fill2[3];
  s16 pc;
  u16 fill3[3];
  s16 pd;
} ObjAffine;

typedef struct {
  u16 color;
  int prio;
  bool mosaic;
  bool blend;
} ObjBufferEntry;

typedef struct {
  u16 color;
  int idx;
  int prio;
} Layer;

typedef enum { OBJMODE_REG, OBJMODE_AFF, OBJMODE_HIDE, OBJMODE_AFFDBL } ObjMode;

void ppu_init(Ppu *ppu);
void ppu_step(Gba *gba, int cycles);
