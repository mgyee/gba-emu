#include "ppu.h"
#include "io.h"
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

static void render_mode3(PPU *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * 240);
  u16 *vram_ptr = (u16 *)ppu->vram + (ppu->LCD.vcount * 240);

  for (int x = 0; x < 240; x++) {
    u16 color = vram_ptr[x];
    dest[x] = rgb15_to_argb(color);
  }
}

static void render_mode4(PPU *ppu) {
  u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * 240);
  u16 *vram_ptr = (u16 *)ppu->vram + (ppu->LCD.vcount * 120);

  for (int x = 0; x < 240; x++) {
    u8 index;
    if (x % 2 == 0) {
      index = vram_ptr[x / 2] & 0xFF;
    } else {
      index = (vram_ptr[x / 2] >> 8) & 0xFF;
    }
    u16 color = ((u16 *)ppu->palram)[index];
    dest[x] = rgb15_to_argb(color);
  }
}

void ppu_init(PPU *ppu) { memset(ppu, 0, sizeof(PPU)); }

static void render_scanline(PPU *ppu) {
  if (ppu->LCD.dispcnt.forced_blank) {
    u32 *dest = ppu->framebuffer + (ppu->LCD.vcount * 240);
    for (int i = 0; i < 240; i++)
      dest[i] = 0xFFFFFFFF;
    return;
  }

  switch (ppu->LCD.dispcnt.mode) {
  case 3:
    render_mode3(ppu);
    break;
  case 4:
    render_mode4(ppu);
    break;
  default:
    break;
  }
}

void ppu_step(PPU *ppu, int cycles) {
  ppu->cycle += cycles;

  while (ppu->cycle >= CYCLES_PER_SCANLINE) {
    ppu->cycle -= CYCLES_PER_SCANLINE;
    ppu->LCD.vcount++;

    if (ppu->LCD.vcount < VISIBLE_SCANLINES) {
      render_scanline(ppu);
    } else if (ppu->LCD.vcount >= SCANLINES_PER_FRAME) {
      ppu->LCD.vcount = 0;
    }

    if (ppu->LCD.vcount >= VISIBLE_SCANLINES) {
      ppu->LCD.dispstat.val |= 1;
      ppu->LCD.dispstat.vblank = 1;
    } else {
      ppu->LCD.dispstat.val &= ~1;
      ppu->LCD.dispstat.vblank = 0;
    }

    if (ppu->cycle >= H_VISIBLE_CYCLES) {
      ppu->LCD.dispstat.val |= 2;
      ppu->LCD.dispstat.hblank = 1;
    } else {
      ppu->LCD.dispstat.val &= ~2;
      ppu->LCD.dispstat.hblank = 0;
    }

    if (ppu->LCD.vcount == ppu->LCD.dispstat.vcount_setting) {
      ppu->LCD.dispstat.val |= 4;
      ppu->LCD.dispstat.vblank_irq = 1;
    } else {
      ppu->LCD.dispstat.val &= ~4;
      ppu->LCD.dispstat.vblank_irq = 0;
    }
  }
}
