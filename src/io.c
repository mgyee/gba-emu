#include "io.h"
#include "bus.h"
#include "gba.h"
#include <stdio.h>
#include <string.h>

void io_init(Io *io) { memset(io, 0, sizeof(Io)); }

u8 io_read8(Gba *gba, u32 addr) {
  Ppu *ppu = &gba->ppu;
  Keypad *keypad = &gba->keypad;
  Io *io = &gba->io;
  InterruptManager *int_mgr = &gba->int_mgr;

  switch (addr) {
  case DISPCNT:
    return ppu->LCD.dispcnt.val & 0xFF;
  case DISPCNT + 1:
    return (ppu->LCD.dispcnt.val >> 8) & 0xFF;
  case GREENSWAP:
    return ppu->LCD.greenswap & 0xFF;
  case GREENSWAP + 1:
    return (ppu->LCD.greenswap >> 8) & 0xFF;
  case DISPSTAT:
    return ppu->LCD.dispstat.val & 0xFF;
  case VCOUNT:
    return ppu->LCD.vcount & 0xFF;
  case VCOUNT + 1:
    return (ppu->LCD.vcount >> 8) & 0xFF;
  case DISPSTAT + 1:
    return (ppu->LCD.dispstat.val >> 8) & 0xFF;
  case BG0CNT:
    return ppu->LCD.bgcnt[0].val & 0xFF;
  case BG0CNT + 1:
    return (ppu->LCD.bgcnt[0].val >> 8) & 0xFF;
  case BG1CNT:
    return ppu->LCD.bgcnt[1].val & 0xFF;
  case BG1CNT + 1:
    return (ppu->LCD.bgcnt[1].val >> 8) & 0xFF;
  case BG2CNT:
    return ppu->LCD.bgcnt[2].val & 0xFF;
  case BG2CNT + 1:
    return (ppu->LCD.bgcnt[2].val >> 8) & 0xFF;
  case BG3CNT:
    return ppu->LCD.bgcnt[3].val & 0xFF;
  case BG3CNT + 1:
    return (ppu->LCD.bgcnt[3].val >> 8) & 0xFF;
  // case BG0HOFS:
  //   return bus->ppu->LCD.bghofs[0] & 0xFF;
  // case BG0HOFS + 1:
  //   return (bus->ppu->LCD.bghofs[0] >> 8) & 0xFF;
  // case BG0VOFS:
  //   return bus->ppu->LCD.bgvofs[0] & 0xFF;
  // case BG0VOFS + 1:
  //   return (bus->ppu->LCD.bgvofs[0] >> 8) & 0xFF;
  // case BG1HOFS:
  //   return bus->ppu->LCD.bghofs[1] & 0xFF;
  // case BG1HOFS + 1:
  //   return (bus->ppu->LCD.bghofs[1] >> 8) & 0xFF;
  // case BG1VOFS:
  //   return bus->ppu->LCD.bgvofs[1] & 0xFF;
  // case BG1VOFS + 1:
  //   return (bus->ppu->LCD.bgvofs[1] >> 8) & 0xFF;
  // case BG2HOFS:
  //   return bus->ppu->LCD.bghofs[2] & 0xFF;
  // case BG2HOFS + 1:
  //   return (bus->ppu->LCD.bghofs[2] >> 8) & 0xFF;
  // case BG2VOFS:
  //   return bus->ppu->LCD.bgvofs[2] & 0xFF;
  // case BG2VOFS + 1:
  //   return (bus->ppu->LCD.bgvofs[2] >> 8) & 0xFF;
  // case BG3HOFS:
  //   return bus->ppu->LCD.bghofs[3] & 0xFF;
  // case BG3HOFS + 1:
  //   return (bus->ppu->LCD.bghofs[3] >> 8) & 0xFF;
  // case BG3VOFS:
  //   return bus->ppu->LCD.bgvofs[3] & 0xFF;
  // case BG3VOFS + 1:
  //   return (bus->ppu->LCD.bgvofs[3] >> 8) & 0xFF;
  // case BG2PA:
  //   return bus->ppu->LCD.bgpa[0] & 0xFF;
  // case BG2PA + 1:
  //   return (bus->ppu->LCD.bgpa[0] >> 8) & 0xFF;
  // case BG2PB:
  //   return bus->ppu->LCD.bgpb[0] & 0xFF;
  // case BG2PB + 1:
  //   return (bus->ppu->LCD.bgpb[0] >> 8) & 0xFF;
  // case BG2PC:
  //   return bus->ppu->LCD.bgpc[0] & 0xFF;
  // case BG2PC + 1:
  //   return (bus->ppu->LCD.bgpc[0] >> 8) & 0xFF;
  // case BG2PD:
  //   return bus->ppu->LCD.bgpd[0] & 0xFF;
  // case BG2PD + 1:
  //   return (bus->ppu->LCD.bgpd[0] >> 8) & 0xFF;
  // case BG2X:
  //   return bus->ppu->LCD.bgx[0] & 0xFF;
  // case BG2X + 1:
  //   return (bus->ppu->LCD.bgx[0] >> 8) & 0xFF;
  // case BG2X + 2:
  //   return (bus->ppu->LCD.bgx[0] >> 16) & 0xFF;
  // case BG2X + 3:
  //   return (bus->ppu->LCD.bgx[0] >> 24) & 0xFF;
  // case BG2Y:
  //   return bus->ppu->LCD.bgy[0] & 0xFF;
  // case BG2Y + 1:
  //   return (bus->ppu->LCD.bgy[0] >> 8) & 0xFF;
  // case BG2Y + 2:
  //   return (bus->ppu->LCD.bgy[0] >> 16) & 0xFF;
  // case BG2Y + 3:
  //   return (bus->ppu->LCD.bgy[0] >> 24) & 0xFF;
  // case BG3PA:
  //   return bus->ppu->LCD.bgpa[1] & 0xFF;
  // case BG3PA + 1:
  //   return (bus->ppu->LCD.bgpa[1] >> 8) & 0xFF;
  // case BG3PB:
  //   return bus->ppu->LCD.bgpb[1] & 0xFF;
  // case BG3PB + 1:
  //   return (bus->ppu->LCD.bgpb[1] >> 8) & 0xFF;
  // case BG3PC:
  //   return bus->ppu->LCD.bgpc[1] & 0xFF;
  // case BG3PC + 1:
  //   return (bus->ppu->LCD.bgpc[1] >> 8) & 0xFF;
  // case BG3PD:
  //   return bus->ppu->LCD.bgpd[1] & 0xFF;
  // case BG3PD + 1:
  //   return (bus->ppu->LCD.bgpd[1] >> 8) & 0xFF;
  // case BG3X:
  //   return bus->ppu->LCD.bgx[1] & 0xFF;
  // case BG3X + 1:
  //   return (bus->ppu->LCD.bgx[1] >> 8) & 0xFF;
  // case BG3X + 2:
  //   return (bus->ppu->LCD.bgx[1] >> 16) & 0xFF;
  // case BG3X + 3:
  //   return (bus->ppu->LCD.bgx[1] >> 24) & 0xFF;
  // case BG3Y:
  //   return bus->ppu->LCD.bgy[1] & 0xFF;
  // case BG3Y + 1:
  //   return (bus->ppu->LCD.bgy[1] >> 8) & 0xFF;
  // case BG3Y + 2:
  //   return (bus->ppu->LCD.bgy[1] >> 16) & 0xFF;
  // case BG3Y + 3:
  //   return (bus->ppu->LCD.bgy[1] >> 24) & 0xFF;
  // case WIN0H:
  //   return bus->ppu->LCD.winh[0] & 0xFF;
  // case WIN0H + 1:
  //   return (bus->ppu->LCD.winh[0] >> 8) & 0xFF;
  // case WIN1H:
  //   return bus->ppu->LCD.winh[1] & 0xFF;
  // case WIN1H + 1:
  //   return (bus->ppu->LCD.winh[1] >> 8) & 0xFF;
  // case WIN0V:
  //   return bus->ppu->LCD.winv[0] & 0xFF;
  // case WIN0V + 1:
  //   return (bus->ppu->LCD.winv[0] >> 8) & 0xFF;
  // case WIN1V:
  //   return bus->ppu->LCD.winv[1] & 0xFF;
  // case WIN1V + 1:
  //   return (bus->ppu->LCD.winv[1] >> 8) & 0xFF;
  // case WININ:
  //   return bus->ppu->LCD.winin.val & 0xFF;
  case WININ + 1:
    return (ppu->LCD.winin.val >> 8) & 0xFF;
  case WINOUT:
    return ppu->LCD.winout.val & 0xFF;
  case WINOUT + 1:
    return (ppu->LCD.winout.val >> 8) & 0xFF;
  case MOSAIC:
    return ppu->LCD.mosaic.val & 0xFF;
  case MOSAIC + 1:
    return (ppu->LCD.mosaic.val >> 8) & 0xFF;
  case BLDCNT:
    return ppu->LCD.blendcnt.val & 0xFF;
  case BLDCNT + 1:
    return (ppu->LCD.blendcnt.val >> 8) & 0xFF;
  case BLDALPHA:
    return ppu->LCD.eva & 0xFF;
  case BLDALPHA + 1:
    return ppu->LCD.evb & 0xFF;
  case BLDY:
    return ppu->LCD.evy & 0xFF;

  /* Keypad */
  case KEYINPUT:
    return keypad->keyinput & 0xFF;
  case KEYINPUT + 1:
    return (keypad->keyinput >> 8) & 0xFF;
  case KEYCNT:
    return keypad->keycnt & 0xFF;
  case KEYCNT + 1:
    return (keypad->keycnt >> 8) & 0xFF;

  case IE:
    return int_mgr->ie & 0xFF;
  case IE + 1:
    return (int_mgr->ie >> 8) & 0xFF;
  case IF:
    return int_mgr->if_ & 0xFF;
  case IF + 1:
    return (int_mgr->if_ >> 8) & 0xFF;
  case IME:
    return int_mgr->ime & 0xFF;
  case IME + 1:
    return (int_mgr->ime >> 8) & 0xFF;

  case WAITCNT:
    return io->waitcnt & 0xFF;
  case WAITCNT + 1:
    return (io->waitcnt >> 8) & 0xFF;
  default:
    return 0;
  }
}

void io_write8(Gba *gba, u32 addr, u8 val) {
  Ppu *ppu = &gba->ppu;
  Keypad *keypad = &gba->keypad;
  Io *io = &gba->io;
  InterruptManager *int_mgr = &gba->int_mgr;

  switch (addr) {
  case DISPCNT:
    ppu->LCD.dispcnt.val = (ppu->LCD.dispcnt.val & 0xFF00) | val;
    ppu->LCD.dispcnt.mode = val & 0x7;
    ppu->LCD.dispcnt.cg_mode = (val >> 3) & 1;
    ppu->LCD.dispcnt.frame = (val >> 4) & 1;
    ppu->LCD.dispcnt.hblank_oam_access = (val >> 5) & 1;
    ppu->LCD.dispcnt.oam_mapping_1d = (val >> 6) & 1;
    ppu->LCD.dispcnt.forced_blank = (val >> 7) & 1;
    break;
  case DISPCNT + 1:
    ppu->LCD.dispcnt.val = (ppu->LCD.dispcnt.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 8; i++) {
      ppu->LCD.dispcnt.enable[i] = (val >> i) & 1;
    }
    break;
  case GREENSWAP:
    ppu->LCD.greenswap = (ppu->LCD.greenswap & 0xFF00) | val;
    break;
  case GREENSWAP + 1:
    ppu->LCD.greenswap = (ppu->LCD.greenswap & 0x00FF) | (val << 8);
    break;
  case DISPSTAT:
    ppu->LCD.dispstat.val = (ppu->LCD.dispstat.val & 0xFF00) | val;
    ppu->LCD.dispstat.vblank_irq = (val >> 3) & 1;
    ppu->LCD.dispstat.hblank_irq = (val >> 4) & 1;
    ppu->LCD.dispstat.vcounter_irq = (val >> 5) & 1;
    break;
  case DISPSTAT + 1:
    ppu->LCD.dispstat.val = (ppu->LCD.dispstat.val & 0x00FF) | (val << 8);
    ppu->LCD.dispstat.vcount_setting = val;
    break;
  case BG0CNT:
    ppu->LCD.bgcnt[0].val = (ppu->LCD.bgcnt[0].val & 0xFF00) | val;
    ppu->LCD.bgcnt[0].priority = val & 0x3;
    ppu->LCD.bgcnt[0].char_base_block = (val >> 2) & 0x3;
    ppu->LCD.bgcnt[0].mosaic = (val >> 6) & 1;
    ppu->LCD.bgcnt[0].colors = (val >> 7) & 1;
    break;
  case BG1CNT:
    ppu->LCD.bgcnt[1].val = (ppu->LCD.bgcnt[1].val & 0xFF00) | val;
    ppu->LCD.bgcnt[1].priority = val & 0x3;
    ppu->LCD.bgcnt[1].char_base_block = (val >> 2) & 0x3;
    ppu->LCD.bgcnt[1].mosaic = (val >> 6) & 1;
    ppu->LCD.bgcnt[1].colors = (val >> 7) & 1;
    break;
  case BG2CNT:
    ppu->LCD.bgcnt[2].val = (ppu->LCD.bgcnt[2].val & 0xFF00) | val;
    ppu->LCD.bgcnt[2].priority = val & 0x3;
    ppu->LCD.bgcnt[2].char_base_block = (val >> 2) & 0x3;
    ppu->LCD.bgcnt[2].mosaic = (val >> 6) & 1;
    ppu->LCD.bgcnt[2].colors = (val >> 7) & 1;
    break;
  case BG3CNT:
    ppu->LCD.bgcnt[3].val = (ppu->LCD.bgcnt[3].val & 0xFF00) | val;
    ppu->LCD.bgcnt[3].priority = val & 0x3;
    ppu->LCD.bgcnt[3].char_base_block = (val >> 2) & 0x3;
    ppu->LCD.bgcnt[3].mosaic = (val >> 6) & 1;
    ppu->LCD.bgcnt[3].colors = (val >> 7) & 1;
    break;
  case BG0CNT + 1:
    ppu->LCD.bgcnt[0].val = (ppu->LCD.bgcnt[0].val & 0x00FF) | (val << 8);
    ppu->LCD.bgcnt[0].screen_base_block = val & 0x1F;
    ppu->LCD.bgcnt[0].display_area_overflow = (val >> 5) & 1;
    ppu->LCD.bgcnt[0].screen_size = (val >> 6) & 3;
    break;
  case BG1CNT + 1:
    ppu->LCD.bgcnt[1].val = (ppu->LCD.bgcnt[1].val & 0x00FF) | (val << 8);
    ppu->LCD.bgcnt[1].screen_base_block = val & 0x1F;
    ppu->LCD.bgcnt[1].display_area_overflow = (val >> 5) & 1;
    ppu->LCD.bgcnt[1].screen_size = (val >> 6) & 3;
    break;
  case BG2CNT + 1:
    ppu->LCD.bgcnt[2].val = (ppu->LCD.bgcnt[2].val & 0x00FF) | (val << 8);
    ppu->LCD.bgcnt[2].screen_base_block = val & 0x1F;
    ppu->LCD.bgcnt[2].display_area_overflow = (val >> 5) & 1;
    ppu->LCD.bgcnt[2].screen_size = (val >> 6) & 3;
    break;
  case BG3CNT + 1:
    ppu->LCD.bgcnt[3].val = (ppu->LCD.bgcnt[3].val & 0x00FF) | (val << 8);
    ppu->LCD.bgcnt[3].screen_base_block = val & 0x1F;
    ppu->LCD.bgcnt[3].display_area_overflow = (val >> 5) & 1;
    ppu->LCD.bgcnt[3].screen_size = (val >> 6) & 3;
    break;
  case BG0HOFS:
    ppu->LCD.bghofs[0] = (ppu->LCD.bghofs[0] & 0xFF00) | val;
    break;
  case BG0HOFS + 1:
    ppu->LCD.bghofs[0] = (ppu->LCD.bghofs[0] & 0x00FF) | (val << 8);
    break;
  case BG0VOFS:
    ppu->LCD.bgvofs[0] = (ppu->LCD.bgvofs[0] & 0xFF00) | val;
    break;
  case BG0VOFS + 1:
    ppu->LCD.bgvofs[0] = (ppu->LCD.bgvofs[0] & 0x00FF) | (val << 8);
    break;
  case BG1HOFS:
    ppu->LCD.bghofs[1] = (ppu->LCD.bghofs[1] & 0xFF00) | val;
    break;
  case BG1HOFS + 1:
    ppu->LCD.bghofs[1] = (ppu->LCD.bghofs[1] & 0x00FF) | (val << 8);
    break;
  case BG1VOFS:
    ppu->LCD.bgvofs[1] = (ppu->LCD.bgvofs[1] & 0xFF00) | val;
    break;
  case BG1VOFS + 1:
    ppu->LCD.bgvofs[1] = (ppu->LCD.bgvofs[1] & 0x00FF) | (val << 8);
    break;
  case BG2HOFS:
    ppu->LCD.bghofs[2] = (ppu->LCD.bghofs[2] & 0xFF00) | val;
    break;
  case BG2HOFS + 1:
    ppu->LCD.bghofs[2] = (ppu->LCD.bghofs[2] & 0x00FF) | (val << 8);
    break;
  case BG2VOFS:
    ppu->LCD.bgvofs[2] = (ppu->LCD.bgvofs[2] & 0xFF00) | val;
    break;
  case BG2VOFS + 1:
    ppu->LCD.bgvofs[2] = (ppu->LCD.bgvofs[2] & 0x00FF) | (val << 8);
    break;
  case BG3HOFS:
    ppu->LCD.bghofs[3] = (ppu->LCD.bghofs[3] & 0xFF00) | val;
    break;
  case BG3HOFS + 1:
    ppu->LCD.bghofs[3] = (ppu->LCD.bghofs[3] & 0x00FF) | (val << 8);
    break;
  case BG3VOFS:
    ppu->LCD.bgvofs[3] = (ppu->LCD.bgvofs[3] & 0xFF00) | val;
    break;
  case BG3VOFS + 1:
    ppu->LCD.bgvofs[3] = (ppu->LCD.bgvofs[3] & 0x00FF) | (val << 8);
    break;
  case BG2PA:
    ppu->LCD.bgpa[0] = (ppu->LCD.bgpa[0] & 0xFF00) | val;
    break;
  case BG2PA + 1:
    ppu->LCD.bgpa[0] = (ppu->LCD.bgpa[0] & 0x00FF) | (val << 8);
    break;
  case BG2PB:
    ppu->LCD.bgpb[0] = (ppu->LCD.bgpb[0] & 0xFF00) | val;
    break;
  case BG2PB + 1:
    ppu->LCD.bgpb[0] = (ppu->LCD.bgpb[0] & 0x00FF) | (val << 8);
    break;
  case BG2PC:
    ppu->LCD.bgpc[0] = (ppu->LCD.bgpc[0] & 0xFF00) | val;
    break;
  case BG2PC + 1:
    ppu->LCD.bgpc[0] = (ppu->LCD.bgpc[0] & 0x00FF) | (val << 8);
    break;
  case BG2PD:
    ppu->LCD.bgpd[0] = (ppu->LCD.bgpd[0] & 0xFF00) | val;
    break;
  case BG2PD + 1:
    ppu->LCD.bgpd[0] = (ppu->LCD.bgpd[0] & 0x00FF) | (val << 8);
    break;
  case BG2X:
    ppu->LCD.bgx[0].internal = (ppu->LCD.bgx[0].internal & 0xFFFFFF00) | val;
    break;
  case BG2X + 1:
    ppu->LCD.bgx[0].internal =
        (ppu->LCD.bgx[0].internal & 0xFFFF00FF) | (val << 8);
    break;
  case BG2X + 2:
    ppu->LCD.bgx[0].internal =
        (ppu->LCD.bgx[0].internal & 0xFF00FFFF) | (val << 16);
    break;
  case BG2X + 3:
    ppu->LCD.bgx[0].internal =
        (ppu->LCD.bgx[0].internal & 0x00FFFFFF) | (val << 24);
    break;
  case BG2Y:
    ppu->LCD.bgy[0].internal = (ppu->LCD.bgy[0].internal & 0xFFFFFF00) | val;
    break;
  case BG2Y + 1:
    ppu->LCD.bgy[0].internal =
        (ppu->LCD.bgy[0].internal & 0xFFFF00FF) | (val << 8);
    break;
  case BG2Y + 2:
    ppu->LCD.bgy[0].internal =
        (ppu->LCD.bgy[0].internal & 0xFF00FFFF) | (val << 16);
    break;
  case BG2Y + 3:
    ppu->LCD.bgy[0].internal =
        (ppu->LCD.bgy[0].internal & 0x00FFFFFF) | (val << 24);
    break;
  case BG3PA:
    ppu->LCD.bgpa[1] = (ppu->LCD.bgpa[1] & 0xFF00) | val;
    break;
  case BG3PA + 1:
    ppu->LCD.bgpa[1] = (ppu->LCD.bgpa[1] & 0x00FF) | (val << 8);
    break;
  case BG3PB:
    ppu->LCD.bgpb[1] = (ppu->LCD.bgpb[1] & 0xFF00) | val;
    break;
  case BG3PB + 1:
    ppu->LCD.bgpb[1] = (ppu->LCD.bgpb[1] & 0x00FF) | (val << 8);
    break;
  case BG3PC:
    ppu->LCD.bgpc[1] = (ppu->LCD.bgpc[1] & 0xFF00) | val;
    break;
  case BG3PC + 1:
    ppu->LCD.bgpc[1] = (ppu->LCD.bgpc[1] & 0x00FF) | (val << 8);
    break;
  case BG3PD:
    ppu->LCD.bgpd[1] = (ppu->LCD.bgpd[1] & 0xFF00) | val;
    break;
  case BG3PD + 1:
    ppu->LCD.bgpd[1] = (ppu->LCD.bgpd[1] & 0x00FF) | (val << 8);
    break;
  case BG3X:
    ppu->LCD.bgx[1].internal = (ppu->LCD.bgx[1].internal & 0xFFFFFF00) | val;
    break;
  case BG3X + 1:
    ppu->LCD.bgx[1].internal =
        (ppu->LCD.bgx[1].internal & 0xFFFF00FF) | (val << 8);
    break;
  case BG3X + 2:
    ppu->LCD.bgx[1].internal =
        (ppu->LCD.bgx[1].internal & 0xFF00FFFF) | (val << 16);
    break;
  case BG3X + 3:
    ppu->LCD.bgx[1].internal =
        (ppu->LCD.bgx[1].internal & 0x00FFFFFF) | (val << 24);
    break;
  case BG3Y:
    ppu->LCD.bgy[1].internal = (ppu->LCD.bgy[1].internal & 0xFFFFFF00) | val;
    break;
  case BG3Y + 1:
    ppu->LCD.bgy[1].internal =
        (ppu->LCD.bgy[1].internal & 0xFFFF00FF) | (val << 8);
    break;
  case BG3Y + 2:
    ppu->LCD.bgy[1].internal =
        (ppu->LCD.bgy[1].internal & 0xFF00FFFF) | (val << 16);
    break;
  case BG3Y + 3:
    ppu->LCD.bgy[1].internal =
        (ppu->LCD.bgy[1].internal & 0x00FFFFFF) | (val << 24);
    break;
  case WIN0H:
    ppu->LCD.winh[0] = (ppu->LCD.winh[0] & 0xFF00) | val;
    break;
  case WIN0H + 1:
    ppu->LCD.winh[0] = (ppu->LCD.winh[0] & 0x00FF) | (val << 8);
    break;
  case WIN1H:
    ppu->LCD.winh[1] = (ppu->LCD.winh[1] & 0xFF00) | val;
    break;
  case WIN1H + 1:
    ppu->LCD.winh[1] = (ppu->LCD.winh[1] & 0x00FF) | (val << 8);
    break;
  case WIN0V:
    ppu->LCD.winv[0] = (ppu->LCD.winv[0] & 0xFF00) | val;
    break;
  case WIN0V + 1:
    ppu->LCD.winv[0] = (ppu->LCD.winv[0] & 0x00FF) | (val << 8);
    break;
  case WIN1V:
    ppu->LCD.winv[1] = (ppu->LCD.winv[1] & 0xFF00) | val;
    break;
  case WIN1V + 1:
    ppu->LCD.winv[1] = (ppu->LCD.winv[1] & 0x00FF) | (val << 8);
    break;
  case WININ:
    ppu->LCD.winin.val = (ppu->LCD.winin.val & 0xFF00) | val;
    for (int i = 0; i < 6; i++) {
      ppu->LCD.winin.enable[0][i] = (val >> i) & 1;
    }
    break;
  case WININ + 1:
    ppu->LCD.winin.val = (ppu->LCD.winin.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->LCD.winin.enable[1][i] = (val >> i) & 1;
    }
    break;
  case WINOUT:
    ppu->LCD.winout.val = (ppu->LCD.winout.val & 0xFF00) | val;
    for (int i = 0; i < 6; i++) {
      ppu->LCD.winout.enable[0][i] = (val >> i) & 1;
    }
    break;
  case WINOUT + 1:
    ppu->LCD.winout.val = (ppu->LCD.winout.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->LCD.winout.enable[1][i] = (val >> i) & 1;
    }
    break;
  case MOSAIC:
    ppu->LCD.mosaic.val = (ppu->LCD.mosaic.val & 0xFF00) | val;
    ppu->LCD.mosaic.bg_h = val & 0xF;
    ppu->LCD.mosaic.bg_v = (val >> 4) & 0xF;
    break;
  case MOSAIC + 1:
    ppu->LCD.mosaic.val = (ppu->LCD.mosaic.val & 0x00FF) | (val << 8);
    ppu->LCD.mosaic.obj_h = val & 0xF;
    ppu->LCD.mosaic.obj_v = (val >> 4) & 0xF;
    break;
  case BLDCNT:
    ppu->LCD.blendcnt.val = (ppu->LCD.blendcnt.val & 0xFF00) | val;
    ppu->LCD.blendcnt.effect = val & 0x3;
    for (int i = 0; i < 6; i++) {
      ppu->LCD.blendcnt.targets[0][i] = (val >> i) & 1;
    }
    break;
  case BLDCNT + 1:
    ppu->LCD.blendcnt.val = (ppu->LCD.blendcnt.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->LCD.blendcnt.targets[1][i] = (val >> i) & 1;
    }
    break;
  case BLDALPHA:
    ppu->LCD.eva = val & 0x1F;
    break;
  case BLDALPHA + 1:
    ppu->LCD.evb = val & 0x1F;
    break;
  case BLDY:
    ppu->LCD.evy = val & 0x1F;
    break;

  /* Keypad */
  case KEYCNT:
    keypad->keycnt = (keypad->keycnt & 0xFF00) | val;
    break;
  case KEYCNT + 1:
    keypad->keycnt = (keypad->keycnt & 0x00FF) | (val << 8);
    break;

  case IE:
    int_mgr->ie = (int_mgr->ie & 0xFF00) | val;
    break;
  case IE + 1:
    int_mgr->ie = (int_mgr->ie & 0x00FF) | (val << 8);
    break;
  case IF:
    int_mgr->if_ = (int_mgr->if_ & 0xFF00) | val;
    break;
  case IF + 1:
    int_mgr->if_ = (int_mgr->if_ & 0x00FF) | (val << 8);
    break;
  case IME:
    int_mgr->ime = (int_mgr->ime & 0xFF00) | val;
    break;
  case IME + 1:
    int_mgr->ime = (int_mgr->ime & 0x00FF) | (val << 8);
    break;

  case WAITCNT:
    io->waitcnt = (io->waitcnt & 0xFF00) | val;
    bus_update_waitstates(&gba->bus, io->waitcnt);
    break;
  case WAITCNT + 1:
    io->waitcnt = (io->waitcnt & 0x00FF) | (val << 8);
    bus_update_waitstates(&gba->bus, io->waitcnt);
    break;
  default:
    break;
  }
}

u16 io_read16(Gba *gba, u32 addr) {
  return io_read8(gba, addr) | (io_read8(gba, addr + 1) << 8);
}

void io_write16(Gba *gba, u32 addr, u16 val) {
  switch (addr) {
  case KEYCNT:
    gba->keypad.keycnt = val;
    break;
  case WAITCNT:
    gba->io.waitcnt = val;
    bus_update_waitstates(&gba->bus, val);
    break;
  default:
    io_write8(gba, addr, val & 0xFF);
    io_write8(gba, addr + 1, (val >> 8) & 0xFF);
    break;
  }
}

u32 io_read32(Gba *gba, u32 addr) {
  return io_read16(gba, addr) | (io_read16(gba, addr + 2) << 16);
}

void io_write32(Gba *gba, u32 addr, u32 val) {
  io_write16(gba, addr, val & 0xFFFF);
  io_write16(gba, addr + 2, (val >> 16) & 0xFFFF);
}
