#include "io.h"
#include "bus.h"
#include "gba.h"
#include "ppu.h"
#include <stdio.h>
#include <string.h>

void io_init(Io *io) {
  memset(io, 0, sizeof(Io));
  io->power_state = POWER_STATE_NORMAL;
}

u8 io_read8(Gba *gba, u32 addr) {
  Ppu *ppu = &gba->ppu;
  Keypad *keypad = &gba->keypad;
  Io *io = &gba->io;
  InterruptManager *int_mgr = &gba->int_mgr;

  switch (addr) {
  case DISPCNT:
    return ppu->Lcd.dispcnt.val & 0xFF;
  case DISPCNT + 1:
    return (ppu->Lcd.dispcnt.val >> 8) & 0xFF;
  case GREENSWAP:
    return ppu->Lcd.greenswap & 0xFF;
  case GREENSWAP + 1:
    return (ppu->Lcd.greenswap >> 8) & 0xFF;
  case DISPSTAT:
    return ppu->Lcd.dispstat.val & 0xFF;
  case VCOUNT:
    return ppu->Lcd.vcount & 0xFF;
  case VCOUNT + 1:
    return (ppu->Lcd.vcount >> 8) & 0xFF;
  case DISPSTAT + 1:
    return (ppu->Lcd.dispstat.val >> 8) & 0xFF;
  case BG0CNT:
    return ppu->Lcd.bgcnt[0].val & 0xFF;
  case BG0CNT + 1:
    return (ppu->Lcd.bgcnt[0].val >> 8) & 0xFF;
  case BG1CNT:
    return ppu->Lcd.bgcnt[1].val & 0xFF;
  case BG1CNT + 1:
    return (ppu->Lcd.bgcnt[1].val >> 8) & 0xFF;
  case BG2CNT:
    return ppu->Lcd.bgcnt[2].val & 0xFF;
  case BG2CNT + 1:
    return (ppu->Lcd.bgcnt[2].val >> 8) & 0xFF;
  case BG3CNT:
    return ppu->Lcd.bgcnt[3].val & 0xFF;
  case BG3CNT + 1:
    return (ppu->Lcd.bgcnt[3].val >> 8) & 0xFF;
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
    return (ppu->Lcd.winin.val >> 8) & 0xFF;
  case WINOUT:
    return ppu->Lcd.winout.val & 0xFF;
  case WINOUT + 1:
    return (ppu->Lcd.winout.val >> 8) & 0xFF;
  case MOSAIC:
    return ppu->Lcd.mosaic.val & 0xFF;
  case MOSAIC + 1:
    return (ppu->Lcd.mosaic.val >> 8) & 0xFF;
  case BLDCNT:
    return ppu->Lcd.blendcnt.val & 0xFF;
  case BLDCNT + 1:
    return (ppu->Lcd.blendcnt.val >> 8) & 0xFF;
  case BLDALPHA:
    return ppu->Lcd.eva & 0xFF;
  case BLDALPHA + 1:
    return ppu->Lcd.evb & 0xFF;
  case BLDY:
    return ppu->Lcd.evy & 0xFF;

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
    ppu->Lcd.dispcnt.val = (ppu->Lcd.dispcnt.val & 0xFF00) | val;
    ppu->Lcd.dispcnt.mode = val & 0x7;
    ppu->Lcd.dispcnt.cg_mode = (val >> 3) & 1;
    ppu->Lcd.dispcnt.frame = (val >> 4) & 1;
    ppu->Lcd.dispcnt.hblank_oam_access = (val >> 5) & 1;
    ppu->Lcd.dispcnt.oam_mapping_1d = (val >> 6) & 1;
    ppu->Lcd.dispcnt.forced_blank = (val >> 7) & 1;
    break;
  case DISPCNT + 1:
    ppu->Lcd.dispcnt.val = (ppu->Lcd.dispcnt.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 8; i++) {
      ppu->Lcd.dispcnt.enable[i] = (val >> i) & 1;
    }
    break;
  case GREENSWAP:
    ppu->Lcd.greenswap = (ppu->Lcd.greenswap & 0xFF00) | val;
    break;
  case GREENSWAP + 1:
    ppu->Lcd.greenswap = (ppu->Lcd.greenswap & 0x00FF) | (val << 8);
    break;
  case DISPSTAT:
    // lower 3 bits are read only
    ppu->Lcd.dispstat.val = (ppu->Lcd.dispstat.val & 0xFF07) | (val & 0xF8);
    ppu->Lcd.dispstat.vblank_irq = (val >> 3) & 1;
    ppu->Lcd.dispstat.hblank_irq = (val >> 4) & 1;
    ppu->Lcd.dispstat.vcounter_irq = (val >> 5) & 1;
    break;
  case DISPSTAT + 1:
    ppu->Lcd.dispstat.val = (ppu->Lcd.dispstat.val & 0x00FF) | (val << 8);
    ppu->Lcd.dispstat.vcount_setting = val;
    break;
  case BG0CNT:
    ppu->Lcd.bgcnt[0].val = (ppu->Lcd.bgcnt[0].val & 0xFF00) | val;
    ppu->Lcd.bgcnt[0].priority = val & 0x3;
    ppu->Lcd.bgcnt[0].char_base_block = (val >> 2) & 0x3;
    ppu->Lcd.bgcnt[0].mosaic = (val >> 6) & 1;
    ppu->Lcd.bgcnt[0].colors = (val >> 7) & 1;
    break;
  case BG1CNT:
    ppu->Lcd.bgcnt[1].val = (ppu->Lcd.bgcnt[1].val & 0xFF00) | val;
    ppu->Lcd.bgcnt[1].priority = val & 0x3;
    ppu->Lcd.bgcnt[1].char_base_block = (val >> 2) & 0x3;
    ppu->Lcd.bgcnt[1].mosaic = (val >> 6) & 1;
    ppu->Lcd.bgcnt[1].colors = (val >> 7) & 1;
    break;
  case BG2CNT:
    ppu->Lcd.bgcnt[2].val = (ppu->Lcd.bgcnt[2].val & 0xFF00) | val;
    ppu->Lcd.bgcnt[2].priority = val & 0x3;
    ppu->Lcd.bgcnt[2].char_base_block = (val >> 2) & 0x3;
    ppu->Lcd.bgcnt[2].mosaic = (val >> 6) & 1;
    ppu->Lcd.bgcnt[2].colors = (val >> 7) & 1;
    break;
  case BG3CNT:
    ppu->Lcd.bgcnt[3].val = (ppu->Lcd.bgcnt[3].val & 0xFF00) | val;
    ppu->Lcd.bgcnt[3].priority = val & 0x3;
    ppu->Lcd.bgcnt[3].char_base_block = (val >> 2) & 0x3;
    ppu->Lcd.bgcnt[3].mosaic = (val >> 6) & 1;
    ppu->Lcd.bgcnt[3].colors = (val >> 7) & 1;
    break;
  case BG0CNT + 1:
    ppu->Lcd.bgcnt[0].val = (ppu->Lcd.bgcnt[0].val & 0x00FF) | (val << 8);
    ppu->Lcd.bgcnt[0].screen_base_block = val & 0x1F;
    ppu->Lcd.bgcnt[0].display_area_overflow = (val >> 5) & 1;
    ppu->Lcd.bgcnt[0].screen_size = (val >> 6) & 3;
    break;
  case BG1CNT + 1:
    ppu->Lcd.bgcnt[1].val = (ppu->Lcd.bgcnt[1].val & 0x00FF) | (val << 8);
    ppu->Lcd.bgcnt[1].screen_base_block = val & 0x1F;
    ppu->Lcd.bgcnt[1].display_area_overflow = (val >> 5) & 1;
    ppu->Lcd.bgcnt[1].screen_size = (val >> 6) & 3;
    break;
  case BG2CNT + 1:
    ppu->Lcd.bgcnt[2].val = (ppu->Lcd.bgcnt[2].val & 0x00FF) | (val << 8);
    ppu->Lcd.bgcnt[2].screen_base_block = val & 0x1F;
    ppu->Lcd.bgcnt[2].display_area_overflow = (val >> 5) & 1;
    ppu->Lcd.bgcnt[2].screen_size = (val >> 6) & 3;
    break;
  case BG3CNT + 1:
    ppu->Lcd.bgcnt[3].val = (ppu->Lcd.bgcnt[3].val & 0x00FF) | (val << 8);
    ppu->Lcd.bgcnt[3].screen_base_block = val & 0x1F;
    ppu->Lcd.bgcnt[3].display_area_overflow = (val >> 5) & 1;
    ppu->Lcd.bgcnt[3].screen_size = (val >> 6) & 3;
    break;
  case BG0HOFS:
    ppu->Lcd.bghofs[0] = (ppu->Lcd.bghofs[0] & 0xFF00) | val;
    break;
  case BG0HOFS + 1:
    ppu->Lcd.bghofs[0] = (ppu->Lcd.bghofs[0] & 0x00FF) | (val << 8);
    break;
  case BG0VOFS:
    ppu->Lcd.bgvofs[0] = (ppu->Lcd.bgvofs[0] & 0xFF00) | val;
    break;
  case BG0VOFS + 1:
    ppu->Lcd.bgvofs[0] = (ppu->Lcd.bgvofs[0] & 0x00FF) | (val << 8);
    break;
  case BG1HOFS:
    ppu->Lcd.bghofs[1] = (ppu->Lcd.bghofs[1] & 0xFF00) | val;
    break;
  case BG1HOFS + 1:
    ppu->Lcd.bghofs[1] = (ppu->Lcd.bghofs[1] & 0x00FF) | (val << 8);
    break;
  case BG1VOFS:
    ppu->Lcd.bgvofs[1] = (ppu->Lcd.bgvofs[1] & 0xFF00) | val;
    break;
  case BG1VOFS + 1:
    ppu->Lcd.bgvofs[1] = (ppu->Lcd.bgvofs[1] & 0x00FF) | (val << 8);
    break;
  case BG2HOFS:
    ppu->Lcd.bghofs[2] = (ppu->Lcd.bghofs[2] & 0xFF00) | val;
    break;
  case BG2HOFS + 1:
    ppu->Lcd.bghofs[2] = (ppu->Lcd.bghofs[2] & 0x00FF) | (val << 8);
    break;
  case BG2VOFS:
    ppu->Lcd.bgvofs[2] = (ppu->Lcd.bgvofs[2] & 0xFF00) | val;
    break;
  case BG2VOFS + 1:
    ppu->Lcd.bgvofs[2] = (ppu->Lcd.bgvofs[2] & 0x00FF) | (val << 8);
    break;
  case BG3HOFS:
    ppu->Lcd.bghofs[3] = (ppu->Lcd.bghofs[3] & 0xFF00) | val;
    break;
  case BG3HOFS + 1:
    ppu->Lcd.bghofs[3] = (ppu->Lcd.bghofs[3] & 0x00FF) | (val << 8);
    break;
  case BG3VOFS:
    ppu->Lcd.bgvofs[3] = (ppu->Lcd.bgvofs[3] & 0xFF00) | val;
    break;
  case BG3VOFS + 1:
    ppu->Lcd.bgvofs[3] = (ppu->Lcd.bgvofs[3] & 0x00FF) | (val << 8);
    break;
  case BG2PA:
    ppu->Lcd.bgpa[0] = (ppu->Lcd.bgpa[0] & 0xFF00) | val;
    break;
  case BG2PA + 1:
    ppu->Lcd.bgpa[0] = (ppu->Lcd.bgpa[0] & 0x00FF) | (val << 8);
    break;
  case BG2PB:
    ppu->Lcd.bgpb[0] = (ppu->Lcd.bgpb[0] & 0xFF00) | val;
    break;
  case BG2PB + 1:
    ppu->Lcd.bgpb[0] = (ppu->Lcd.bgpb[0] & 0x00FF) | (val << 8);
    break;
  case BG2PC:
    ppu->Lcd.bgpc[0] = (ppu->Lcd.bgpc[0] & 0xFF00) | val;
    break;
  case BG2PC + 1:
    ppu->Lcd.bgpc[0] = (ppu->Lcd.bgpc[0] & 0x00FF) | (val << 8);
    break;
  case BG2PD:
    ppu->Lcd.bgpd[0] = (ppu->Lcd.bgpd[0] & 0xFF00) | val;
    break;
  case BG2PD + 1:
    ppu->Lcd.bgpd[0] = (ppu->Lcd.bgpd[0] & 0x00FF) | (val << 8);
    break;
  case BG2X:
    ppu->Lcd.bgx[0].current = (ppu->Lcd.bgx[0].current & 0xFFFFFF00) | val;
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    break;
  case BG2X + 1:
    ppu->Lcd.bgx[0].current =
        (ppu->Lcd.bgx[0].current & 0xFFFF00FF) | (val << 8);
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    break;
  case BG2X + 2:
    ppu->Lcd.bgx[0].current =
        (ppu->Lcd.bgx[0].current & 0xFF00FFFF) | (val << 16);
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    break;
  case BG2X + 3:
    ppu->Lcd.bgx[0].current =
        (ppu->Lcd.bgx[0].current & 0x00FFFFFF) | (val << 24);
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    break;
  case BG2Y:
    ppu->Lcd.bgy[0].current = (ppu->Lcd.bgy[0].current & 0xFFFFFF00) | val;
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    break;
  case BG2Y + 1:
    ppu->Lcd.bgy[0].current =
        (ppu->Lcd.bgy[0].current & 0xFFFF00FF) | (val << 8);
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    break;
  case BG2Y + 2:
    ppu->Lcd.bgy[0].current =
        (ppu->Lcd.bgy[0].current & 0xFF00FFFF) | (val << 16);
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    break;
  case BG2Y + 3:
    ppu->Lcd.bgy[0].current =
        (ppu->Lcd.bgy[0].current & 0x00FFFFFF) | (val << 24);
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    break;
  case BG3PA:
    ppu->Lcd.bgpa[1] = (ppu->Lcd.bgpa[1] & 0xFF00) | val;
    break;
  case BG3PA + 1:
    ppu->Lcd.bgpa[1] = (ppu->Lcd.bgpa[1] & 0x00FF) | (val << 8);
    break;
  case BG3PB:
    ppu->Lcd.bgpb[1] = (ppu->Lcd.bgpb[1] & 0xFF00) | val;
    break;
  case BG3PB + 1:
    ppu->Lcd.bgpb[1] = (ppu->Lcd.bgpb[1] & 0x00FF) | (val << 8);
    break;
  case BG3PC:
    ppu->Lcd.bgpc[1] = (ppu->Lcd.bgpc[1] & 0xFF00) | val;
    break;
  case BG3PC + 1:
    ppu->Lcd.bgpc[1] = (ppu->Lcd.bgpc[1] & 0x00FF) | (val << 8);
    break;
  case BG3PD:
    ppu->Lcd.bgpd[1] = (ppu->Lcd.bgpd[1] & 0xFF00) | val;
    break;
  case BG3PD + 1:
    ppu->Lcd.bgpd[1] = (ppu->Lcd.bgpd[1] & 0x00FF) | (val << 8);
    break;
  case BG3X:
    ppu->Lcd.bgx[1].current = (ppu->Lcd.bgx[1].current & 0xFFFFFF00) | val;
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    break;
  case BG3X + 1:
    ppu->Lcd.bgx[1].current =
        (ppu->Lcd.bgx[1].current & 0xFFFF00FF) | (val << 8);
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    break;
  case BG3X + 2:
    ppu->Lcd.bgx[1].current =
        (ppu->Lcd.bgx[1].current & 0xFF00FFFF) | (val << 16);
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    break;
  case BG3X + 3:
    ppu->Lcd.bgx[1].current =
        (ppu->Lcd.bgx[1].current & 0x00FFFFFF) | (val << 24);
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    break;
  case BG3Y:
    ppu->Lcd.bgy[1].current = (ppu->Lcd.bgy[1].current & 0xFFFFFF00) | val;
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;
    break;
  case BG3Y + 1:
    ppu->Lcd.bgy[1].current =
        (ppu->Lcd.bgy[1].current & 0xFFFF00FF) | (val << 8);
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;
    break;
  case BG3Y + 2:
    ppu->Lcd.bgy[1].current =
        (ppu->Lcd.bgy[1].current & 0xFF00FFFF) | (val << 16);
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;
    break;
  case BG3Y + 3:
    ppu->Lcd.bgy[1].current =
        (ppu->Lcd.bgy[1].current & 0x00FFFFFF) | (val << 24);
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;
    break;
  case WIN0H:
    ppu->Lcd.winh[0] = (ppu->Lcd.winh[0] & 0xFF00) | val;
    break;
  case WIN0H + 1:
    ppu->Lcd.winh[0] = (ppu->Lcd.winh[0] & 0x00FF) | (val << 8);
    break;
  case WIN1H:
    ppu->Lcd.winh[1] = (ppu->Lcd.winh[1] & 0xFF00) | val;
    break;
  case WIN1H + 1:
    ppu->Lcd.winh[1] = (ppu->Lcd.winh[1] & 0x00FF) | (val << 8);
    break;
  case WIN0V:
    ppu->Lcd.winv[0] = (ppu->Lcd.winv[0] & 0xFF00) | val;
    break;
  case WIN0V + 1:
    ppu->Lcd.winv[0] = (ppu->Lcd.winv[0] & 0x00FF) | (val << 8);
    break;
  case WIN1V:
    ppu->Lcd.winv[1] = (ppu->Lcd.winv[1] & 0xFF00) | val;
    break;
  case WIN1V + 1:
    ppu->Lcd.winv[1] = (ppu->Lcd.winv[1] & 0x00FF) | (val << 8);
    break;
  case WININ:
    ppu->Lcd.winin.val = (ppu->Lcd.winin.val & 0xFF00) | val;
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winin.win0[i] = (val >> i) & 1;
    }
    break;
  case WININ + 1:
    ppu->Lcd.winin.val = (ppu->Lcd.winin.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winin.win1[i] = (val >> i) & 1;
    }
    break;
  case WINOUT:
    ppu->Lcd.winout.val = (ppu->Lcd.winout.val & 0xFF00) | val;
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winout.win_out[i] = (val >> i) & 1;
    }
    break;
  case WINOUT + 1:
    ppu->Lcd.winout.val = (ppu->Lcd.winout.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winout.win_obj[i] = (val >> i) & 1;
    }
    break;
  case MOSAIC:
    ppu->Lcd.mosaic.val = (ppu->Lcd.mosaic.val & 0xFF00) | val;
    ppu->Lcd.mosaic.bg_h = val & 0xF;
    ppu->Lcd.mosaic.bg_v = (val >> 4) & 0xF;
    break;
  case MOSAIC + 1:
    ppu->Lcd.mosaic.val = (ppu->Lcd.mosaic.val & 0x00FF) | (val << 8);
    ppu->Lcd.mosaic.obj_h = val & 0xF;
    ppu->Lcd.mosaic.obj_v = (val >> 4) & 0xF;
    break;
  case BLDCNT:
    ppu->Lcd.blendcnt.val = (ppu->Lcd.blendcnt.val & 0xFF00) | val;
    ppu->Lcd.blendcnt.effect = (val >> 6) & 0x3;
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.blendcnt.targets[0][i] = (val >> i) & 1;
    }
    break;
  case BLDCNT + 1:
    ppu->Lcd.blendcnt.val = (ppu->Lcd.blendcnt.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.blendcnt.targets[1][i] = (val >> i) & 1;
    }
    break;
  case BLDALPHA:
    ppu->Lcd.eva = val & 0x1F;
    break;
  case BLDALPHA + 1:
    ppu->Lcd.evb = val & 0x1F;
    break;
  case BLDY:
    ppu->Lcd.evy = val & 0x1F;
    break;

  /* Keypad */
  case KEYCNT:
    keypad->keycnt = (keypad->keycnt & 0xFF00) | val;
    break;
  case KEYCNT + 1:
    keypad->keycnt = (keypad->keycnt & 0x00FF) | (val << 8);
    break;

  /* Interrupt */
  case IE:
    int_mgr->ie = (int_mgr->ie & 0xFF00) | val;
    break;
  case IE + 1:
    int_mgr->ie = (int_mgr->ie & 0x00FF) | (val << 8);
    break;
  case IF:
    int_mgr->if_ &= ~val;
    break;
  case IF + 1:
    int_mgr->if_ &= ~(val << 8);
    break;
  case IME:
    int_mgr->ime = (int_mgr->ime & 0xFF00) | val;
    break;
  case IME + 1:
    int_mgr->ime = (int_mgr->ime & 0x00FF) | (val << 8);
    break;

  case HALTCNT:
    if (val & 0x80) {
      io->power_state = POWER_STATE_STOPPED;
    } else {
      io->power_state = POWER_STATE_HALTED;
    }
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
  Ppu *ppu = &gba->ppu;
  Io *io = &gba->io;
  Keypad *keypad = &gba->keypad;

  switch (addr) {
  case BG2X:
    ppu->Lcd.bgx[0].current = (ppu->Lcd.bgx[0].current & 0xFFFF0000) | val;
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    break;
  case BG2X + 2:
    ppu->Lcd.bgx[0].current =
        (ppu->Lcd.bgx[0].current & 0x0000FFFF) | (val << 16);
    ppu->Lcd.bgx[0].internal = ppu->Lcd.bgx[0].current;
    break;
  case BG2Y:
    ppu->Lcd.bgy[0].current = (ppu->Lcd.bgy[0].current & 0xFFFF0000) | val;
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    break;
  case BG2Y + 2:
    ppu->Lcd.bgy[0].current =
        (ppu->Lcd.bgy[0].current & 0x0000FFFF) | (val << 16);
    ppu->Lcd.bgy[0].internal = ppu->Lcd.bgy[0].current;
    break;
  case BG3X:
    ppu->Lcd.bgx[1].current = (ppu->Lcd.bgx[1].current & 0xFFFF0000) | val;
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    break;
  case BG3X + 2:
    ppu->Lcd.bgx[1].current =
        (ppu->Lcd.bgx[1].current & 0x0000FFFF) | (val << 16);
    ppu->Lcd.bgx[1].internal = ppu->Lcd.bgx[1].current;
    break;
  case BG3Y:
    ppu->Lcd.bgy[1].current = (ppu->Lcd.bgy[1].current & 0xFFFF0000) | val;
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;
    break;
  case BG3Y + 2:
    ppu->Lcd.bgy[1].current =
        (ppu->Lcd.bgy[1].current & 0x0000FFFF) | (val << 16);
    ppu->Lcd.bgy[1].internal = ppu->Lcd.bgy[1].current;
    break;

    // Keypad
  case KEYCNT:
    keypad->keycnt = val;
    break;
  case WAITCNT:
    io->waitcnt = val;
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
  Ppu *ppu = &gba->ppu;

  switch (addr) {
  case BG2X:
    ppu->Lcd.bgx[0].current = val;
    ppu->Lcd.bgx[0].internal = val;
    break;
  case BG2Y:
    ppu->Lcd.bgy[0].current = val;
    ppu->Lcd.bgy[0].internal = val;
    break;
  case BG3X:
    ppu->Lcd.bgx[1].current = val;
    ppu->Lcd.bgx[1].internal = val;
    break;
  case BG3Y:
    ppu->Lcd.bgy[1].current = val;
    ppu->Lcd.bgy[1].internal = val;
    break;
  }
  io_write16(gba, addr, val & 0xFFFF);
  io_write16(gba, addr + 2, (val >> 16) & 0xFFFF);
}
