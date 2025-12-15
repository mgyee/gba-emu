#pragma once
#include "common.h"

struct PPU {
  u32 framebuffer[240 * 160];
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
      enum Effect {
        BLEND_NONE,
        BLEND_ALPHA,
        BLEND_BRIGHTEN,
        BLEND_DARKEN,
      } effect;
      u16 val;
      int targets[2][6];
      int color_special_effect;
    } blendcnt;

    int eva;
    int evb;
    int evy;
  } LCD;
};

void ppu_init(PPU *ppu);
void ppu_step(PPU *ppu, int cycles);
