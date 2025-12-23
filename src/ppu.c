#include "ppu.h"
#include "common.h"
#include "gba.h"
#include <string.h>

#define CYCLES_PER_SCANLINE 1232
#define SCANLINES_PER_FRAME 228
#define VISIBLE_SCANLINES 160
#define H_VISIBLE_CYCLES 960
#define H_BLANK_CYCLES 272

// Line Buffer Flags
#define OBJ_PIXEL_PRESENT 0x80000000
#define OBJ_PIXEL_MOSAIC 0x40000000
#define OBJ_COLOR_MASK 0x00FFFFFF

// [Shape][Size] -> {Width, Height}
static const int sizes[3][4][2] = {{{8, 8}, {16, 16}, {32, 32}, {64, 64}},
                                   {{16, 8}, {32, 8}, {32, 16}, {64, 32}},
                                   {{8, 16}, {8, 32}, {16, 32}, {32, 64}}};

static inline u32 rgb15_to_argb(u16 color) {
  u32 r = (color & 0x1F);
  u32 g = (color >> 5) & 0x1F;
  u32 b = (color >> 10) & 0x1F;

  r = (r << 3) | (r >> 2);
  g = (g << 3) | (g >> 2);
  b = (b << 3) | (b >> 2);

  return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static void render_obj_reg(Ppu *ppu, ObjAttr *obj, u32 *line_buffer) {
  u8 *tile_base = ppu->vram + 0x10000;

  int screen_y = ppu->LCD.vcount;

  u16 attr0 = obj->attr[0];
  u16 attr1 = obj->attr[1];
  u16 attr2 = obj->attr[2];

  int disable = TEST_BIT(attr0, 9);
  if (disable) {
    return;
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
    return;
  }

  int mosaic = TEST_BIT(attr0, 12);
  int color_mode = TEST_BIT(attr0, 13);
  int hf = TEST_BIT(attr1, 12);
  int vf = TEST_BIT(attr1, 13);
  int tile_idx = GET_BITS(attr2, 0, 10);
  int pal_bank = GET_BITS(attr2, 12, 4);

  int sprite_y = screen_y - obj_y;
  if (mosaic) {
    int mos_v = ppu->LCD.mosaic.obj_v + 1;
    sprite_y -= sprite_y % mos_v;
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
    int sprite_x = x - obj_x;
    if (hf) {
      sprite_x = width - sprite_x - 1;
    }

    int tile_x = sprite_x / 8;
    int subtile_x = sprite_x % 8;

    int curr_tile = tile_start + (1 + color_mode) * tile_x;

    int tile_addr = ((curr_tile % 1024) * 32);

    int color_idx;
    if (color_mode) {
      tile_addr += (subtile_y * 8) + subtile_x;
      color_idx = tile_base[tile_addr];
    } else {
      tile_addr += (subtile_y * 4) + (subtile_x / 2);
      color_idx = tile_base[tile_addr];
      if (subtile_x & 1) {
        color_idx >>= 4;
      } else {
        color_idx &= 0xF;
      }
    }

    if (color_idx != 0) {
      u16 color =
          ((u16 *)ppu
               ->palram)[0x100 + color_idx + (!color_mode * (pal_bank * 16))];
      u32 val = rgb15_to_argb(color) & OBJ_COLOR_MASK;
      val |= OBJ_PIXEL_PRESENT;
      line_buffer[x] = val;
    }

    if (mosaic) {
      line_buffer[x] |= OBJ_PIXEL_MOSAIC;
    }
  }
}

static void render_obj_aff(Ppu *ppu, ObjAttr *obj, u32 *line_buffer) {
  u8 *tile_base = ppu->vram + 0x10000;

  int screen_y = ppu->LCD.vcount;

  u16 attr0 = obj->attr[0];
  u16 attr1 = obj->attr[1];
  u16 attr2 = obj->attr[2];

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

  int mode = GET_BITS(attr0, 8, 2);
  int draw_width = width;
  int draw_height = height;

  if (mode == OBJMODE_AFFDBL) {
    draw_width *= 2;
    draw_height *= 2;
  }

  int center_x = width / 2;
  int center_y = height / 2;
  int draw_center_x = draw_width / 2;
  int draw_center_y = draw_height / 2;

  int left = MIN(PIXELS_WIDTH, MAX(obj_x, 0));
  int right = MIN(PIXELS_WIDTH, MAX(obj_x + draw_width, 0));

  if (screen_y < obj_y || screen_y >= (obj_y + draw_height)) {
    return;
  }

  int mosaic = TEST_BIT(attr0, 12);
  int color_mode = TEST_BIT(attr0, 13);
  int aff_idx = GET_BITS(attr1, 9, 5);
  int tile_idx = GET_BITS(attr2, 0, 10);
  int pal_bank = GET_BITS(attr2, 12, 4);

  ObjAffine *oam = (ObjAffine *)ppu->oam;
  ObjAffine aff = oam[aff_idx];

  int sprite_y = screen_y - obj_y;
  if (mosaic) {
    int mos_v = ppu->LCD.mosaic.obj_v + 1;
    sprite_y -= sprite_y % mos_v;
  }

  int dy = sprite_y - draw_center_y;

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

  for (int x = left; x < right; x++) {
    int sprite_x = x - obj_x;

    int dx = sprite_x - draw_center_x;

    int tex_x = ((aff.pa * dx + aff.pb * dy) >> 8) + center_x;
    int tex_y = ((aff.pc * dx + aff.pd * dy) >> 8) + center_y;

    if (tex_x < 0 || tex_x >= width || tex_y < 0 || tex_y >= height) {
      continue;
    }

    int tile_x = tex_x >> 3;
    int subtile_x = tex_x % 8;

    int tile_y = tex_y >> 3;
    int subtile_y = tex_y % 8;

    int tile_start = tile_idx + (tile_y * tile_stride);

    int curr_tile = tile_start + (1 + color_mode) * tile_x;

    int tile_addr = ((curr_tile % 1024) * 32);

    int color_idx;
    if (color_mode) {
      tile_addr += (subtile_y * 8) + subtile_x;
      color_idx = tile_base[tile_addr];
    } else {
      tile_addr += (subtile_y * 4) + (subtile_x / 2);
      color_idx = tile_base[tile_addr];
      if (subtile_x & 1) {
        color_idx >>= 4;
      } else {
        color_idx &= 0xF;
      }
    }

    if (color_idx != 0) {
      u16 color =
          ((u16 *)ppu
               ->palram)[0x100 + color_idx + (!color_mode * (pal_bank * 16))];
      u32 val = rgb15_to_argb(color) & OBJ_COLOR_MASK;

      val |= OBJ_PIXEL_PRESENT;
      line_buffer[x] = val;
    }

    if (mosaic) {
      line_buffer[x] |= OBJ_PIXEL_MOSAIC;
    }
  }
}

static void render_objs(Ppu *ppu, int prio) {
  if (!ppu->LCD.dispcnt.enable[4]) {
    return;
  }

  ObjAttr *oam = (ObjAttr *)ppu->oam;

  u32 line_buffer[PIXELS_WIDTH];
  memset(line_buffer, 0, sizeof(line_buffer));

  for (int i = 127; i >= 0; i--) {
    ObjAttr *obj = &oam[i];
    int obj_prio = GET_BITS(obj->attr[2], 10, 2);
    int gfx_mode = GET_BITS(obj->attr[0], 10, 2);

    if (obj_prio != prio || gfx_mode == GFXMODE_FORBIDDEN) {
      continue;
    }

    ObjMode mode = GET_BITS(obj->attr[0], 8, 2);

    switch (mode) {
    case OBJMODE_HIDE:
      continue;
    case OBJMODE_REG:
      render_obj_reg(ppu, obj, line_buffer);
      break;
    case OBJMODE_AFF:
    case OBJMODE_AFFDBL:
      render_obj_aff(ppu, obj, line_buffer);
      break;
    }

    int mos_h = ppu->LCD.mosaic.obj_h + 1;

    if (mos_h > 1) {
      for (int x = 0; x < PIXELS_WIDTH; x++) {
        u32 pixel = line_buffer[x];
        if (pixel & OBJ_PIXEL_MOSAIC) {
          line_buffer[x] = line_buffer[x - (x % mos_h)];
        }
      }
    }

    u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);
    for (int x = 0; x < PIXELS_WIDTH; x++) {
      u32 pixel = line_buffer[x];
      if (pixel & OBJ_PIXEL_PRESENT) {
        dest[x] = 0xFF000000 | (pixel & OBJ_COLOR_MASK);
      }
    }
  }
}

static void render_bg_reg(Ppu *ppu, int i, int prio) {
  if (!ppu->LCD.dispcnt.enable[i] || (ppu->LCD.bgcnt[i].priority != prio)) {
    return;
  }

  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  u16 *map_base =
      (u16 *)(ppu->vram + (ppu->LCD.bgcnt[i].screen_base_block * 0x800));
  int tile_base = ppu->LCD.bgcnt[i].char_base_block * 0x4000;

  int size = ppu->LCD.bgcnt[i].screen_size;
  int width = (size & 1) ? 512 : 256;
  int height = (size & 2) ? 512 : 256;
  int color_mode = ppu->LCD.bgcnt[i].colors;

  int mosaic = ppu->LCD.bgcnt[i].mosaic;

  int screen_y = ppu->LCD.vcount;
  if (mosaic) {
    int mos_v = ppu->LCD.mosaic.bg_v + 1;
    screen_y -= screen_y % mos_v;
  }
  int map_y = (screen_y + ppu->LCD.bgvofs[i]) % height;
  int block_y = map_y >> 8;
  int tile_y = (map_y & 255) >> 3;
  int subtile_y = map_y % 8;

  int mos_h = ppu->LCD.mosaic.bg_h + 1;

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    int screen_x = x;
    if (mosaic) {
      screen_x -= screen_x % mos_h;
    }
    int map_x = (screen_x + ppu->LCD.bghofs[i]) % width;
    int block_x = map_x >> 8;
    int tile_x = (map_x & 255) >> 3;
    int subtile_x = map_x % 8;

    u16 entry = map_base[(block_y * (width >> 8) + block_x) * 1024 +
                         tile_y * 32 + tile_x];

    int tile_idx = GET_BITS(entry, 0, 10);
    bool hf = TEST_BIT(entry, 10);
    bool vf = TEST_BIT(entry, 11);
    int pal_bank = GET_BITS(entry, 12, 4);

    if (hf) {
      subtile_x = 7 - subtile_x;
    }
    if (vf) {
      subtile_y = 7 - subtile_y;
    }

    int color_idx;
    if (color_mode) {
      int tile_addr = tile_base + tile_idx * 64 + subtile_y * 8 + subtile_x;
      if (tile_addr >= 0x10000) {
        continue;
      }
      color_idx = ppu->vram[tile_addr];
    } else {
      int tile_addr = tile_base + tile_idx * 32 + subtile_y * 4 + subtile_x / 2;
      if (tile_addr >= 0x10000) {
        continue;
      }
      color_idx = ppu->vram[tile_addr];
      if (subtile_x % 2) {
        color_idx >>= 4;
      }
      color_idx &= 0x0F;
    }

    if (color_idx != 0) {
      u16 color =
          ((u16 *)ppu->palram)[color_idx + (!color_mode * (pal_bank * 16))];
      dest[x] = rgb15_to_argb(color);
    }
  }
}

static void render_bg_aff(Ppu *ppu, int i, int prio) {
  if (!ppu->LCD.dispcnt.enable[i] || (ppu->LCD.bgcnt[i].priority != prio)) {
    return;
  }

  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * PIXELS_WIDTH);

  u8 *map_base = (ppu->vram + (ppu->LCD.bgcnt[i].screen_base_block * 0x800));
  int tile_base = ppu->LCD.bgcnt[i].char_base_block * 0x4000;

  int size = ppu->LCD.bgcnt[i].screen_size;
  int shift = size + 7;
  int width = 1 << shift;
  int height = 1 << shift;

  int wrap = ppu->LCD.bgcnt[i].display_area_overflow;
  int mosaic = ppu->LCD.bgcnt[i].mosaic;

  int screen_y = ppu->LCD.vcount;
  if (mosaic) {
    int mos_v = ppu->LCD.mosaic.bg_v + 1;
    screen_y -= screen_y % mos_v;
  }

  int pa = ppu->LCD.bgpa[i - 2];
  int pc = ppu->LCD.bgpc[i - 2];
  int cx = ppu->LCD.bgx[i - 2].internal;
  int cy = ppu->LCD.bgy[i - 2].internal;

  u32 latched_color = 0;

  int mos_h = ppu->LCD.mosaic.bg_h + 1;

  for (int x = 0; x < PIXELS_WIDTH; x++) {

    if (mosaic && (x % mos_h) != 0) {
      if (latched_color != 0) {
        dest[x] = latched_color;
      }
      cx += pa;
      cy += pc;
      continue;
    }

    int tex_x = cx >> 8;
    int tex_y = cy >> 8;

    if (wrap) {
      tex_x &= width - 1;
      tex_y &= height - 1;
    }
    if (tex_x < 0 || tex_x >= width || tex_y < 0 || tex_y >= height) {
      cx += pa;
      cy += pc;
      latched_color = 0;
      continue;
    }

    int tile_x = tex_x / 8;
    int tile_y = tex_y / 8;
    int subtile_x = tex_x % 8;
    int subtile_y = tex_y % 8;

    u8 tile_idx = map_base[(tile_y * (width / 8) + tile_x)];
    int tile_addr = tile_base + tile_idx * 64 + subtile_y * 8 + subtile_x;
    int color_idx = ppu->vram[tile_addr];

    if (color_idx != 0) {
      u16 color = ((u16 *)ppu->palram)[color_idx];
      dest[x] = rgb15_to_argb(color);
      latched_color = dest[x];
    } else {
      latched_color = 0;
    }
    cx += pa;
    cy += pc;
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
    render_bg_aff(ppu, 2, prio);
    for (int i = 1; i >= 0; i--) {
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
    for (int i = 3; i >= 2; i--) {
      render_bg_aff(ppu, i, prio);
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

      ppu->LCD.bgx[0].internal += ppu->LCD.bgpb[0];
      ppu->LCD.bgy[0].internal += ppu->LCD.bgpd[0];
      ppu->LCD.bgx[1].internal += ppu->LCD.bgpb[1];
      ppu->LCD.bgy[1].internal += ppu->LCD.bgpd[1];
    } else {
      if (ppu->LCD.vcount == VISIBLE_SCANLINES) {
        ppu->LCD.bgx[0].internal = ppu->LCD.bgx[0].current;
        ppu->LCD.bgy[0].internal = ppu->LCD.bgy[0].current;
        ppu->LCD.bgx[1].internal = ppu->LCD.bgx[1].current;
        ppu->LCD.bgy[1].internal = ppu->LCD.bgy[1].current;
      }
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
