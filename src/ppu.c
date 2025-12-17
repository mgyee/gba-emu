#include "ppu.h"
#include "common.h"
#include "gba.h"
#include <string.h>

#define CYCLES_PER_SCANLINE 1232
#define SCANLINES_PER_FRAME 228
#define VISIBLE_SCANLINES 160
#define H_VISIBLE_CYCLES 960
#define H_BLNANK_CYCLES 272

static inline u32 rgb15_to_argb(u16 color) {
  u32 r = (color & 0x1F);
  u32 g = (color >> 5) & 0x1F;
  u32 b = (color >> 10) & 0x1F;

  r = (r << 3) | (r >> 2);
  g = (g << 3) | (g >> 2);
  b = (b << 3) | (b >> 2);

  return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static void render_objs(Ppu *ppu, int prio) {
  if (!ppu->LCD.dispcnt.enable[4]) {
    return;
  }

  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  ObjAttr *oam = (ObjAttr *)ppu->oam;

  // [Shape][Size] -> {Width, Height}
  static const int sizes[3][4][2] = {{{8, 8}, {16, 16}, {32, 32}, {64, 64}},
                                     {{16, 8}, {32, 8}, {32, 16}, {64, 32}},
                                     {{8, 16}, {8, 32}, {16, 32}, {32, 64}}};

  int screen_y = ppu->LCD.vcount;

  for (int i = 127; i >= 0; i--) {
    ObjAttr *obj = &oam[i];
    u16 attr0 = obj->attr[0];
    u16 attr1 = obj->attr[1];
    u16 attr2 = obj->attr[2];
    int obj_prio = GET_BITS(obj->attr[2], 10, 2);
    int gfx_mode = GET_BITS(attr0, 10, 2);

    if (obj_prio != prio || gfx_mode == GFXMODE_FORBIDDEN) {
      continue;
    }

    ObjMode mode = GET_BITS(obj->attr[0], 8, 2);

    switch (mode) {
    case OBJMODE_HIDE:
      continue;
    case OBJMODE_REG:
      break;
    case OBJMODE_AFF:
    case OBJMODE_AFFDBL:
      break;
    }

    int obj_y = GET_BITS(attr0, 0, 8);
    if (obj_y >= PIXELS_HEIGHT) {
      obj_y -= 256;
    }
    int obj_x = GET_BITS(attr1, 0, 9);
    if (obj_x >= PIXELS_WIDTH) {
      obj_x -= 512;
    }

    int shape = GET_BITS(attr0, 14, 2);
    int size = GET_BITS(attr1, 14, 2);

    int width = sizes[shape][size][0];
    int height = sizes[shape][size][1];

    int left = MIN(PIXELS_WIDTH, MAX(obj_x, 0));
    int right = MIN(PIXELS_WIDTH, MAX(obj_x + width, 0));

    if (screen_y < obj_y || screen_y >= (obj_y + height)) {
      continue;
    }

    int mosaic = TEST_BIT(attr0, 12);
    int color_mode = TEST_BIT(attr0, 13);
    int hf = TEST_BIT(attr1, 12);
    int vf = TEST_BIT(attr1, 13);
    int tile_idx = GET_BITS(attr2, 0, 10);
    int pal_bank = GET_BITS(attr2, 12, 4);

    int sprite_y = screen_y - obj_y;
    if (mosaic) {
      int mosaic_size = ppu->LCD.mosaic.obj_v + 1;
      sprite_y -= sprite_y % mosaic_size;
    }
    if (vf) {
      sprite_y = height - sprite_y - 1;
    }

    int tile_y = sprite_y >> 3;
    int tile_stride;
    if (!ppu->LCD.dispcnt.oam_mapping_1d) {
      tile_stride = 32;
    } else {
      if (color_mode) {
        tile_stride = width >> 2;
      } else {
        tile_stride = width >> 3;
      }
    }

    int tile_start = tile_idx + (tile_y * tile_stride);

    int subtile_y = sprite_y % 8;

    for (int x = left; x < right; x++) {
      int screen_x = x;
      int sprite_x = x - obj_x;
      if (mosaic) {
        int mosaic_size = ppu->LCD.mosaic.obj_h + 1;
        sprite_x -= sprite_x % mosaic_size;
      }
      if (hf) {
        sprite_x = width - sprite_x - 1;
      }

      int tile_x = sprite_x / 8;
      int subtile_x = sprite_x % 8;

      int curr_tile = tile_start + (1 + color_mode) * tile_x;

      int tile_addr = 0x10000 + ((curr_tile % 1024) * 32);

      int idx;
      if (color_mode) {
        tile_addr += (subtile_y * 8) + subtile_x;
        idx = ppu->vram[tile_addr];
      } else {
        tile_addr += (subtile_y * 4) + (subtile_x / 2);
        idx = ppu->vram[tile_addr];
        if (subtile_x & 1) {
          idx >>= 4;
        } else {
          idx &= 0xF;
        }
      }

      if (idx != 0) {
        u16 color =
            ((u16 *)ppu->palram)[0x100 + idx + (!color_mode * (pal_bank * 16))];
        dest[screen_x] = rgb15_to_argb(color);
      }
    }
  }
}

static void render_bg_reg(Ppu *ppu, int i, int prio) {
  if (!ppu->LCD.dispcnt.enable[i] || (ppu->LCD.bgcnt[i].priority != prio)) {
    return;
  }

  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  int char_base = ppu->LCD.bgcnt[i].char_base_block * 0x4000;
  u16 *screen_block =
      (u16 *)(ppu->vram + (ppu->LCD.bgcnt[i].screen_base_block * 0x800));
  int size = ppu->LCD.bgcnt[i].screen_size;
  int width = (size & 1) ? 512 : 256;
  int height = (size & 2) ? 512 : 256;
  int color_mode = ppu->LCD.bgcnt[i].colors;

  int screen_y = ppu->LCD.vcount;
  if (ppu->LCD.bgcnt[i].mosaic) {
    int mosaic_size = ppu->LCD.mosaic.bg_v + 1;
    screen_y -= screen_y % mosaic_size;
  }
  int map_y = (screen_y + ppu->LCD.bgvofs[i]) % height;
  int block_y = map_y >> 8;
  int tile_y = (map_y & 255) >> 3;

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    int screen_x = x;
    if (ppu->LCD.bgcnt[i].mosaic) {
      int mosaic_size = ppu->LCD.mosaic.bg_h + 1;
      screen_x -= screen_x % mosaic_size;
    }
    int map_x = (screen_x + ppu->LCD.bghofs[i]) & (width - 1);
    int block_x = map_x >> 8;
    int tile_x = (map_x & 255) >> 3;

    u16 entry = screen_block[(block_y * (width >> 8) + block_x) * 1024 +
                             tile_y * 32 + tile_x];

    int tile_idx = GET_BITS(entry, 0, 10);
    bool hf = TEST_BIT(entry, 10);
    bool vf = TEST_BIT(entry, 11);
    int pal_bank = GET_BITS(entry, 12, 4);

    int u = map_x & 7;
    int v = map_y & 7;

    if (hf) {
      u = 7 - u;
    }
    if (vf) {
      v = 7 - v;
    }

    int idx;
    if (color_mode) {
      int tile_addr = char_base + tile_idx * 64 + v * 8 + u;
      if (tile_addr >= 0x10000) {
        continue;
      }
      idx = ppu->vram[tile_addr];
    } else {
      int tile_addr = char_base + tile_idx * 32 + v * 4 + u / 2;
      if (tile_addr >= 0x10000) {
        continue;
      }
      idx = ppu->vram[tile_addr];
      if (u % 2) {
        idx >>= 4;
      }
      idx &= 0x0F;
    }

    if (idx != 0) {
      u16 color = ((u16 *)ppu->palram)[idx + (!color_mode * (pal_bank * 16))];
      dest[x] = rgb15_to_argb(color);
    }
  }
}

static void render_mode0(Ppu *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  u16 background_color = ((u16 *)ppu->palram)[0];
  u32 background_argb = rgb15_to_argb(background_color);

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    dest[x] = background_argb;
  }

  for (int prio = 3; prio >= 0; prio--) {
    for (int i = 3; i >= 0; i--) {
      render_bg_reg(ppu, i, prio);
    }
    render_objs(ppu, prio);
  }
}

static void render_mode1(Ppu *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  u16 background_color = ((u16 *)ppu->palram)[0];
  u32 background_argb = rgb15_to_argb(background_color);

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    dest[x] = background_argb;
  }

  for (int prio = 3; prio >= 0; prio--) {
    for (int i = 3; i >= 0; i--) {
      render_bg_reg(ppu, i, prio);
    }
    render_objs(ppu, prio);
  }
}

static void render_mode2(Ppu *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  u16 background_color = ((u16 *)ppu->palram)[0];
  u32 background_argb = rgb15_to_argb(background_color);

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    dest[x] = background_argb;
  }

  for (int prio = 3; prio >= 0; prio--) {
    for (int i = 3; i >= 0; i--) {
      render_bg_reg(ppu, i, prio);
    }
    render_objs(ppu, prio);
  }
}

static void render_mode3(Ppu *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);
  u16 *vram_ptr = (u16 *)ppu->vram + (ppu->LCD.vcount * PIXELS_WIDTH);

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    u16 color = vram_ptr[x];
    dest[x] = rgb15_to_argb(color);
  }
}

static void render_mode4(Ppu *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);
  u8 *vram_ptr = ppu->vram + (ppu->LCD.dispcnt.frame * 0xA000) +
                 (ppu->LCD.vcount * PIXELS_WIDTH);

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    u8 idx = vram_ptr[x];
    u16 color = ((u16 *)ppu->palram)[idx];
    dest[x] = rgb15_to_argb(color);
  }
}

static void render_mode5(Ppu *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);
  if (ppu->LCD.vcount >= 128) {
    for (int x = 0; x < PIXELS_WIDTH; x++) {
      dest[x] = 0xFFFF00FF;
    }
    return;
  }
  u16 *vram_ptr = (u16 *)(ppu->vram + (ppu->LCD.dispcnt.frame * 0xA000)) +
                  (ppu->LCD.vcount * PIXELS_HEIGHT);

  for (int x = 0; x < PIXELS_HEIGHT; x++) {
    u16 color = vram_ptr[x];
    dest[x] = rgb15_to_argb(color);
  }

  for (int x = PIXELS_HEIGHT; x < PIXELS_WIDTH; x++) {
    dest[x] = 0xFFFF00FF;
  }
}

void ppu_init(Ppu *ppu) { memset(ppu, 0, sizeof(Ppu)); }

static void render_scanline(Ppu *ppu) {
  if (ppu->LCD.dispcnt.forced_blank) {
    u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);
    for (int i = 0; i < PIXELS_WIDTH; i++)
      dest[i] = 0xFFFFFFFF;
    return;
  }

  switch (ppu->LCD.dispcnt.mode) {
  case 0:
    render_mode0(ppu);
    break;
  case 1:
    render_mode1(ppu);
    break;
  case 2:
    render_mode2(ppu);
    break;
  case 3:
    render_mode3(ppu);
    break;
  case 4:
    render_mode4(ppu);
    break;
  case 5:
    render_mode5(ppu);
    break;
  default:
    break;
  }
}

void ppu_step(Gba *gba, int cycles) {
  Ppu *ppu = &gba->ppu;
  ppu->cycle += cycles;

  while (ppu->cycle >= CYCLES_PER_SCANLINE) {
    ppu->cycle -= CYCLES_PER_SCANLINE;

    ppu->LCD.dispstat.hblank = 0;
    ppu->LCD.dispstat.val &= ~2;

    if (ppu->LCD.vcount < VISIBLE_SCANLINES) {
      render_scanline(ppu);
    }

    ppu->LCD.vcount++;

    if (ppu->LCD.vcount >= SCANLINES_PER_FRAME) {
      ppu->LCD.vcount = 0;
    }

    if (ppu->LCD.vcount == ppu->LCD.dispstat.vcount_setting) {
      ppu->LCD.dispstat.vcounter = 1;
      ppu->LCD.dispstat.val |= 4;

      if (ppu->LCD.dispstat.vcounter_irq) {
        raise_interrupt(gba, INT_VCOUNT);
      }
    } else {
      ppu->LCD.dispstat.vcounter = 0;
      ppu->LCD.dispstat.val &= ~4;
    }

    if (ppu->LCD.vcount >= VISIBLE_SCANLINES) {
      ppu->LCD.dispstat.val |= 1;
      ppu->LCD.dispstat.vblank = 1;

      if (ppu->LCD.dispstat.vblank_irq) {
        raise_interrupt(gba, INT_VBLANK);
      }
    } else if (ppu->LCD.vcount == 0) {
      ppu->LCD.dispstat.val &= ~1;
      ppu->LCD.dispstat.vblank = 0;
    }

    if (ppu->cycle >= H_VISIBLE_CYCLES) {
      ppu->LCD.dispstat.val |= 2;
      ppu->LCD.dispstat.hblank = 1;
      if (ppu->LCD.dispstat.hblank_irq) {
        raise_interrupt(gba, INT_HBLANK);
      }
    }
  }
}
