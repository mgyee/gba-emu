#include "ppu.h"
#include "common.h"
#include "gba.h"
#include "scheduler.h"
#include <assert.h>
#include <string.h>

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

static void render_obj_reg(Ppu *ppu, ObjAttr *obj,
                           ObjBufferEntry buffer[PIXELS_WIDTH]) {
  u8 *tile_base = ppu->vram + 0x10000;

  int screen_y = ppu->Lcd.vcount;

  u16 attr0 = obj->attr[0];
  u16 attr1 = obj->attr[1];
  u16 attr2 = obj->attr[2];

  bool disable = TEST_BIT(attr0, 9);
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

  int gfx_mode = GET_BITS(attr0, 10, 2);
  bool mosaic = TEST_BIT(attr0, 12);
  bool color_mode = TEST_BIT(attr0, 13);
  bool hf = TEST_BIT(attr1, 12);
  bool vf = TEST_BIT(attr1, 13);
  int tile_idx = GET_BITS(attr2, 0, 10);
  int prio = GET_BITS(attr2, 10, 2);
  int pal_bank = GET_BITS(attr2, 12, 4);

  int sprite_y = screen_y - obj_y;
  if (mosaic) {
    int mos_v = ppu->Lcd.mosaic.obj_v + 1;
    sprite_y -= sprite_y % mos_v;
  }
  if (vf) {
    sprite_y = height - sprite_y - 1;
  }

  int tile_y = sprite_y >> 3;
  int tile_stride;
  if (!ppu->Lcd.dispcnt.oam_mapping_1d) {
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

    if (ppu->Lcd.dispcnt.mode >= 3 && curr_tile < 512) {
      buffer[x].mosaic = mosaic;
      continue;
    }

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

    if (prio < buffer[x].prio) {
      buffer[x].mosaic = mosaic;
    }
    if (color_idx != 0) {
      if (gfx_mode == GFXMODE_WINDOW) {
        buffer[x].window = true;
      } else if (prio < buffer[x].prio) {
        u16 color =
            ((u16 *)ppu
                 ->palram)[0x100 + color_idx + (!color_mode * (pal_bank * 16))];

        buffer[x].color = color;
        buffer[x].prio = prio;
        buffer[x].blend = gfx_mode == GFXMODE_BLEND;
      }
    }
  }
}

static void render_obj_aff(Ppu *ppu, ObjAttr *obj,
                           ObjBufferEntry buffer[PIXELS_WIDTH]) {
  u8 *tile_base = ppu->vram + 0x10000;

  int screen_y = ppu->Lcd.vcount;

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

  int gfx_mode = GET_BITS(attr0, 10, 2);
  bool mosaic = TEST_BIT(attr0, 12);
  bool color_mode = TEST_BIT(attr0, 13);
  int aff_idx = GET_BITS(attr1, 9, 5);
  int tile_idx = GET_BITS(attr2, 0, 10);
  int prio = GET_BITS(attr2, 10, 2);
  int pal_bank = GET_BITS(attr2, 12, 4);

  ObjAffine *oam = (ObjAffine *)ppu->oam;
  ObjAffine aff = oam[aff_idx];

  int sprite_y = screen_y - obj_y;
  if (mosaic) {
    int mos_v = ppu->Lcd.mosaic.obj_v + 1;
    sprite_y -= sprite_y % mos_v;
  }

  int dy = sprite_y - draw_center_y;

  int tile_stride;
  if (!ppu->Lcd.dispcnt.oam_mapping_1d) {
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

    if (ppu->Lcd.dispcnt.mode >= 3 && curr_tile < 512) {
      buffer[x].mosaic = mosaic;
      continue;
    }

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

    if (prio < buffer[x].prio) {
      buffer[x].mosaic = mosaic;
    }
    if (color_idx != 0) {
      if (gfx_mode == GFXMODE_WINDOW) {
        buffer[x].window = true;
      } else if (prio < buffer[x].prio) {
        u16 color =
            ((u16 *)ppu
                 ->palram)[0x100 + color_idx + (!color_mode * (pal_bank * 16))];

        buffer[x].color = color;
        buffer[x].prio = prio;
        buffer[x].blend = gfx_mode == GFXMODE_BLEND;
      }
    }
  }
}

static void render_objs(Ppu *ppu, ObjBufferEntry buffer[PIXELS_WIDTH]) {
  if (!ppu->Lcd.dispcnt.enable[4]) {
    return;
  }

  ObjAttr *oam = (ObjAttr *)ppu->oam;

  for (int i = 0; i < 128; i++) {
    ObjAttr *obj = &oam[i];
    int gfx_mode = GET_BITS(obj->attr[0], 10, 2);

    if (gfx_mode == GFXMODE_FORBIDDEN) {
      continue;
    }

    ObjMode mode = GET_BITS(obj->attr[0], 8, 2);

    switch (mode) {
    case OBJMODE_HIDE:
      continue;
    case OBJMODE_REG:
      render_obj_reg(ppu, obj, buffer);
      break;
    case OBJMODE_AFF:
    case OBJMODE_AFFDBL:
      render_obj_aff(ppu, obj, buffer);
      break;
    }

    int mos_h = ppu->Lcd.mosaic.obj_h + 1;

    if (mos_h > 1) {
      for (int x = 0; x < PIXELS_WIDTH; x++) {
        ObjBufferEntry entry = buffer[x];
        if (entry.mosaic) {
          buffer[x].color = buffer[x - (x % mos_h)].color;
        }
      }
    }
  }
}

static void render_bg_reg(Ppu *ppu, int i, u16 buffer[PIXELS_WIDTH]) {
  if (!ppu->Lcd.dispcnt.enable[i]) {
    return;
  }

  u16 *map_base =
      (u16 *)(ppu->vram + (ppu->Lcd.bgcnt[i].screen_base_block * 0x800));
  int tile_base = ppu->Lcd.bgcnt[i].char_base_block * 0x4000;

  int size = ppu->Lcd.bgcnt[i].screen_size;
  int width = (size & 1) ? 512 : 256;
  int height = (size & 2) ? 512 : 256;
  int color_mode = ppu->Lcd.bgcnt[i].colors;

  int mosaic = ppu->Lcd.bgcnt[i].mosaic;

  int screen_y = ppu->Lcd.vcount;
  if (mosaic) {
    int mos_v = ppu->Lcd.mosaic.bg_v + 1;
    screen_y -= screen_y % mos_v;
  }
  int map_y = (screen_y + ppu->Lcd.bgvofs[i]) % height;
  int block_y = map_y >> 8;
  int tile_y = (map_y & 255) >> 3;

  int mos_h = ppu->Lcd.mosaic.bg_h + 1;

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    int screen_x = x;
    if (mosaic) {
      screen_x -= screen_x % mos_h;
    }
    int map_x = (screen_x + ppu->Lcd.bghofs[i]) % width;
    int block_x = map_x >> 8;
    int tile_x = (map_x & 255) >> 3;

    u16 entry = map_base[(block_y * (width >> 8) + block_x) * 1024 +
                         tile_y * 32 + tile_x];

    int tile_idx = GET_BITS(entry, 0, 10);
    bool hf = TEST_BIT(entry, 10);
    bool vf = TEST_BIT(entry, 11);
    int pal_bank = GET_BITS(entry, 12, 4);

    int subtile_x = map_x % 8;
    if (hf) {
      subtile_x = 7 - subtile_x;
    }

    int subtile_y = map_y % 8;
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
      buffer[x] = color;
    }
  }
}

static void render_bg_aff(Ppu *ppu, int i, u16 buffer[PIXELS_WIDTH]) {
  if (!ppu->Lcd.dispcnt.enable[i]) {
    return;
  }

  u8 *map_base = (ppu->vram + (ppu->Lcd.bgcnt[i].screen_base_block * 0x800));
  int tile_base = ppu->Lcd.bgcnt[i].char_base_block * 0x4000;

  int size = ppu->Lcd.bgcnt[i].screen_size;
  int shift = size + 7;
  int width = 1 << shift;
  int height = 1 << shift;

  int wrap = ppu->Lcd.bgcnt[i].aff_wrap;
  int mosaic = ppu->Lcd.bgcnt[i].mosaic;

  int screen_y = ppu->Lcd.vcount;
  if (mosaic) {
    int mos_v = ppu->Lcd.mosaic.bg_v + 1;
    screen_y -= screen_y % mos_v;
  }

  int pa = ppu->Lcd.bgpa[i - 2];
  int pc = ppu->Lcd.bgpc[i - 2];
  int cx = ppu->Lcd.bgx[i - 2].internal;
  int cy = ppu->Lcd.bgy[i - 2].internal;

  u32 latched_color = TRANSPARENT;

  int mos_h = ppu->Lcd.mosaic.bg_h + 1;

  for (int x = 0; x < PIXELS_WIDTH; x++) {

    if (mosaic && (x % mos_h) != 0) {
      if (latched_color != TRANSPARENT) {
        buffer[x] = latched_color;
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
      latched_color = TRANSPARENT;
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
      latched_color = ((u16 *)ppu->palram)[color_idx];
      buffer[x] = latched_color;
    } else {
      latched_color = TRANSPARENT;
    }
    cx += pa;
    cy += pc;
  }
}

static void render_mode0(Ppu *ppu, u16 bg_buffers[4][PIXELS_WIDTH]) {
  for (int i = 0; i < 4; i++) {
    render_bg_reg(ppu, i, bg_buffers[i]);
  }
}

static void render_mode1(Ppu *ppu, u16 bg_buffers[4][PIXELS_WIDTH]) {
  for (int i = 0; i < 2; i++) {
    render_bg_reg(ppu, i, bg_buffers[i]);
  }
  render_bg_aff(ppu, 2, bg_buffers[2]);
}

static void render_mode2(Ppu *ppu, u16 bg_buffers[4][PIXELS_WIDTH]) {
  for (int i = 2; i < 4; i++) {
    render_bg_aff(ppu, i, bg_buffers[i]);
  }
}

static void render_mode3(Ppu *ppu, u16 bg_buffers[4][PIXELS_WIDTH]) {
  u16 *vram_ptr = (u16 *)ppu->vram + (ppu->Lcd.vcount * PIXELS_WIDTH);
  u16 *buffer = bg_buffers[2];

  if (ppu->Lcd.dispcnt.enable[2]) {
    for (int x = 0; x < PIXELS_WIDTH; x++) {
      buffer[x] = vram_ptr[x];
    }
  }
}

static void render_mode4(Ppu *ppu, u16 bg_buffers[4][PIXELS_WIDTH]) {
  u8 *vram_ptr = ppu->vram + (ppu->Lcd.dispcnt.page * 0xA000) +
                 (ppu->Lcd.vcount * PIXELS_WIDTH);
  u16 *buffer = bg_buffers[2];

  if (ppu->Lcd.dispcnt.enable[2]) {
    for (int x = 0; x < PIXELS_WIDTH; x++) {
      u8 idx = vram_ptr[x];
      buffer[x] = ((u16 *)ppu->palram)[idx];
    }
  }
}

static void render_mode5(Ppu *ppu, u16 bg_buffers[4][PIXELS_WIDTH]) {
  u16 *vram_ptr = (u16 *)(ppu->vram + (ppu->Lcd.dispcnt.page * 0xA000)) +
                  (ppu->Lcd.vcount * PIXELS_HEIGHT);
  u16 *buffer = bg_buffers[2];

  if (ppu->Lcd.dispcnt.enable[2]) {
    if (ppu->Lcd.vcount >= 128) {
      for (int x = 0; x < PIXELS_WIDTH; x++) {
        buffer[x] = 0x7C1F;
      }
      return;
    }
    for (int x = 0; x < PIXELS_HEIGHT; x++) {
      buffer[x] = vram_ptr[x];
    }

    for (int x = PIXELS_HEIGHT; x < PIXELS_WIDTH; x++) {
      buffer[x] = 0x7C1F;
    }
  }
}

void ppu_init(Ppu *ppu) { memset(ppu, 0, sizeof(Ppu)); }

static u16 blend(u16 color_a, u16 color_b, int weight_a, int weight_b) {
  int r_a = (color_a & 0x1F);
  int g_a = (color_a >> 5) & 0x1F;
  int b_a = (color_a >> 10) & 0x1F;

  int r_b = (color_b & 0x1F);
  int g_b = (color_b >> 5) & 0x1F;
  int b_b = (color_b >> 10) & 0x1F;

  int r = MIN(31, ((r_a * weight_a) >> 4) + ((r_b * weight_b) >> 4));
  int g = MIN(31, ((g_a * weight_a) >> 4) + ((g_b * weight_b) >> 4));
  int b = MIN(31, ((b_a * weight_a) >> 4) + ((b_b * weight_b) >> 4));

  return r | g << 5 | b << 10;
}

static bool in_win(int v, int left, int right) {
  if (left <= right) {
    return v >= left && v < right;
  } else {
    return v >= left || v < right;
  }
}

static void render_scanline(Ppu *ppu) {
  int y = ppu->Lcd.vcount;
  if (ppu->Lcd.dispcnt.forced_blank) {
    u32 *dest = ppu->framebuffer + (y * PIXELS_WIDTH);
    for (int i = 0; i < PIXELS_WIDTH; i++)
      dest[i] = 0xFFFFFFFF;
    return;
  }

  ObjBufferEntry obj_buffer[PIXELS_WIDTH];
  memset(obj_buffer, 0, sizeof(obj_buffer));

  u16 bg_buffers[4][PIXELS_WIDTH];

  for (int i = 0; i < PIXELS_WIDTH; i++) {
    obj_buffer[i].color = TRANSPARENT;
    obj_buffer[i].prio = 4;

    bg_buffers[0][i] = TRANSPARENT;
    bg_buffers[1][i] = TRANSPARENT;
    bg_buffers[2][i] = TRANSPARENT;
    bg_buffers[3][i] = TRANSPARENT;
  }

  render_objs(ppu, obj_buffer);

  switch (ppu->Lcd.dispcnt.mode) {
  case 0:
    render_mode0(ppu, bg_buffers);
    break;
  case 1:
    render_mode1(ppu, bg_buffers);
    break;
  case 2:
    render_mode2(ppu, bg_buffers);
    break;
  case 3:
    render_mode3(ppu, bg_buffers);
    break;
  case 4:
    render_mode4(ppu, bg_buffers);
    break;
  case 5:
    render_mode5(ppu, bg_buffers);
    break;
  default:
    assert(false);
    break;
  }

  u32 *dest = ppu->framebuffer + (y * PIXELS_WIDTH);

  u16 backdrop_color = ((u16 *)ppu->palram)[0] & 0x7FFF;
  Layer backdrop = (Layer){backdrop_color, BACKDROP_IDX, 4};

  int order[4];

  int order_idx = 0;
  for (int prio = 0; prio < 4; prio++) {
    for (int i = 0; i < 4; i++) {
      if (ppu->Lcd.bgcnt[i].priority == prio) {
        order[order_idx++] = i;
      }
    }
  }

  bool win0_enable = ppu->Lcd.dispcnt.enable[5];
  bool win1_enable = ppu->Lcd.dispcnt.enable[6];
  bool obj_enable = ppu->Lcd.dispcnt.enable[7];
  bool windows_active = win0_enable || win1_enable || obj_enable;
  int win0_right = GET_BITS(ppu->Lcd.winh[0], 0, 8);
  int win0_left = GET_BITS(ppu->Lcd.winh[0], 8, 8);
  int win1_right = GET_BITS(ppu->Lcd.winh[1], 0, 8);
  int win1_left = GET_BITS(ppu->Lcd.winh[1], 8, 8);

  int win0_bot = GET_BITS(ppu->Lcd.winv[0], 0, 8);
  int win0_top = GET_BITS(ppu->Lcd.winv[0], 8, 8);
  int win1_bot = GET_BITS(ppu->Lcd.winv[1], 0, 8);
  int win1_top = GET_BITS(ppu->Lcd.winv[1], 8, 8);

  bool win0_v = in_win(y, win0_top, win0_bot);
  bool win1_v = in_win(y, win1_top, win1_bot);

  for (int x = 0; x < PIXELS_WIDTH; x++) {
    ObjBufferEntry entry = obj_buffer[x];

    static int windows_disabled[6] = {1, 1, 1, 1, 1, 1};

    int *window = windows_disabled;

    if (windows_active) {
      if (win0_enable && win0_v && in_win(x, win0_left, win0_right)) {
        window = ppu->Lcd.winin.win0;
      } else if (win1_enable && win1_v && in_win(x, win1_left, win1_right)) {
        window = ppu->Lcd.winin.win1;
      } else if (obj_enable && entry.window) {
        window = ppu->Lcd.winout.win_obj;
      } else {
        window = ppu->Lcd.winout.win_out;
      }
    }
    Layer top = backdrop;
    Layer bot = backdrop;
    bool first = true;
    for (int prio = 0; prio < 4; prio++) {
      int bg_idx = order[prio];
      u16 color = bg_buffers[bg_idx][x];
      if (color != TRANSPARENT && window[bg_idx]) {
        if (first) {
          top = (Layer){color, bg_idx, prio};
          first = false;
        } else {
          bot = (Layer){color, bg_idx, prio};
          break;
        }
      }
    }

    if (entry.color != TRANSPARENT && window[OBJ_IDX]) {
      Layer obj_layer = (Layer){entry.color, OBJ_IDX, entry.prio};
      if (entry.prio <= top.prio) {
        bot = top;
        top = obj_layer;
      } else if (entry.prio <= bot.prio) {
        bot = obj_layer;
      }
    }

    bool blend_obj = (top.idx == OBJ_IDX) && entry.blend;

    if (!(blend_obj || window[WIN_BLD_IDX])) {
      dest[x] = rgb15_to_argb(top.color);
      continue;
    }

    bool blend_top = ppu->Lcd.blendcnt.targets[0][top.idx];
    bool blend_bot = ppu->Lcd.blendcnt.targets[1][bot.idx];

    Effect effect = ppu->Lcd.blendcnt.effect;

    if (blend_obj && blend_bot) {
      effect = ALPHA;
    }

    u16 color = top.color;

    int fade = MIN(16, ppu->Lcd.evy);

    if (blend_obj || blend_top) {
      switch (effect) {
      case ALPHA:
        if (blend_bot) {
          color = blend(top.color, bot.color, ppu->Lcd.eva, ppu->Lcd.evb);
        }
        break;
      case BRIGHTEN: {
        color = blend(top.color, WHITE, 16 - fade, fade);
        break;
      }
      case DARKEN: {
        color = blend(top.color, BLACK, 16 - fade, fade);
        break;
      }
      case NONE:
        break;
      }
    }

    dest[x] = rgb15_to_argb(color);
  }
}

void update_vcounter(Gba *gba) {
  Ppu *ppu = &gba->ppu;

  if (ppu->Lcd.vcount == ppu->Lcd.dispstat.vcount_setting) {
    ppu->Lcd.dispstat.vcounter = 1;
    ppu->Lcd.dispstat.val |= 4;

    if (ppu->Lcd.dispstat.vcounter_irq) {
      raise_interrupt(gba, INT_VCOUNT);
    }
  } else {
    ppu->Lcd.dispstat.vcounter = 0;
    ppu->Lcd.dispstat.val &= ~4;
  }
}

void ppu_hblank_start(Gba *gba, uint lateness) {
  Ppu *ppu = &gba->ppu;

  render_scanline(ppu);

  ppu->Lcd.dispstat.hblank = 1;
  ppu->Lcd.dispstat.val |= 2;

  if (ppu->Lcd.dispstat.hblank_irq) {
    raise_interrupt(gba, INT_HBLANK);
  }
  dma_on_hblank(gba);

  scheduler_push_event(&gba->scheduler, EVENT_TYPE_HBLANK_END,
                       H_BLANK_CYCLES - lateness);
}

void ppu_hblank_end(Gba *gba, uint lateness) {
  Ppu *ppu = &gba->ppu;

  ppu->Lcd.dispstat.hblank = 0;
  ppu->Lcd.dispstat.val &= ~2;

  ppu->Lcd.vcount++;

  update_vcounter(gba);

  if (ppu->Lcd.vcount == VISIBLE_SCANLINES) {
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;

    ppu->Lcd.dispstat.vblank = 1;
    ppu->Lcd.dispstat.val |= 1;
    if (ppu->Lcd.dispstat.vblank_irq) {
      raise_interrupt(gba, INT_VBLANK);
    }
    dma_on_vblank(gba);

    scheduler_push_event(&gba->scheduler, EVENT_TYPE_VBLANK_HBLANK_START,
                         H_VISIBLE_CYCLES - lateness);
  } else {
    ppu->Lcd.bgx[0].internal += ppu->Lcd.bgpb[0];
    ppu->Lcd.bgy[0].internal += ppu->Lcd.bgpd[0];
    ppu->Lcd.bgx[1].internal += ppu->Lcd.bgpb[1];
    ppu->Lcd.bgy[1].internal += ppu->Lcd.bgpd[1];

    scheduler_push_event(&gba->scheduler, EVENT_TYPE_HBLANK_START,
                         H_VISIBLE_CYCLES - lateness);
  }
}

void ppu_vblank_hblank_start(Gba *gba, uint lateness) {
  Ppu *ppu = &gba->ppu;

  ppu->Lcd.dispstat.hblank = 1;
  ppu->Lcd.dispstat.val |= 2;

  if (ppu->Lcd.dispstat.hblank_irq) {
    raise_interrupt(gba, INT_HBLANK);
  }

  scheduler_push_event(&gba->scheduler, EVENT_TYPE_VBLANK_HBLANK_END,
                       H_BLANK_CYCLES - lateness);
}

void ppu_vblank_hblank_end(Gba *gba, uint lateness) {
  Ppu *ppu = &gba->ppu;

  ppu->Lcd.dispstat.hblank = 0;
  ppu->Lcd.dispstat.val &= ~2;

  ppu->Lcd.vcount++;

  if (ppu->Lcd.vcount >= SCANLINES_PER_FRAME) {
    ppu->Lcd.vcount = 0;
  }

  update_vcounter(gba);

  if (ppu->Lcd.vcount == 0) {
    ppu->Lcd.dispstat.vblank = 0;
    ppu->Lcd.dispstat.val &= ~1;

    scheduler_push_event(&gba->scheduler, EVENT_TYPE_HBLANK_START,
                         H_VISIBLE_CYCLES - lateness);
  } else {
    scheduler_push_event(&gba->scheduler, EVENT_TYPE_VBLANK_HBLANK_START,
                         H_VISIBLE_CYCLES - lateness);
  }
}
