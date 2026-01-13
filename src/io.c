#include "io.h"
#include "bus.h"
#include "common.h"
#include "dma.h"
#include "gba.h"
#include "interrupt.h"
#include "ppu.h"
#include "scheduler.h"
#include "timer.h"
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
  case WININ:
    return ppu->Lcd.winin.val & 0xFF;
  case WININ + 1:
    return (ppu->Lcd.winin.val >> 8) & 0xFF;
  case WINOUT:
    return ppu->Lcd.winout.val & 0xFF;
  case WINOUT + 1:
    return (ppu->Lcd.winout.val >> 8) & 0xFF;
  case BLDCNT:
    return ppu->Lcd.blendcnt.val & 0xFF;
  case BLDCNT + 1:
    return (ppu->Lcd.blendcnt.val >> 8) & 0xFF;

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
    // printf("unhandled io read: %08X\n", addr);
    return 0;
  }
}

u16 io_read16(Gba *gba, u32 addr) {
  Apu *apu = &gba->apu;
  Dma *dma = &gba->dma;
  TimerManager *tmr_mgr = &gba->tmr_mgr;
  switch (addr) {
  // Sound
  case SOUNDBIAS:
    return apu->soundbias;

  // DMA
  case DMA0CNT_H:
  case DMA1CNT_H:
  case DMA2CNT_H:
  case DMA3CNT_H: {
    int ch = (addr - DMA0CNT_H) / 12;
    return dma->channels[ch].control.val;
  }

  // Timer
  case TM0CNT_L:
  case TM1CNT_L:
  case TM2CNT_L:
  case TM3CNT_L: {
    int tmr = (addr - TM0CNT_L) / 4;
    return timer_get_count(gba, tmr);
    // return tmr_mgr->timers[tmr].count;
  }
  case TM0CNT_H:
  case TM1CNT_H:
  case TM2CNT_H:
  case TM3CNT_H: {
    int tmr = (addr - TM0CNT_H) / 4;
    return tmr_mgr->timers[tmr].control.val;
  }
  }
  return io_read8(gba, addr) | (io_read8(gba, addr + 1) << 8);
}

u32 io_read32(Gba *gba, u32 addr) {
  return io_read16(gba, addr) | (io_read16(gba, addr + 2) << 16);
}

void io_write8(Gba *gba, u32 addr, u8 val) {
  Ppu *ppu = &gba->ppu;
  Keypad *keypad = &gba->keypad;
  Io *io = &gba->io;
  InterruptManager *int_mgr = &gba->int_mgr;

  switch (addr) {
  case DISPCNT:
    ppu->Lcd.dispcnt.val = (ppu->Lcd.dispcnt.val & 0xFF00) | val;
    ppu->Lcd.dispcnt.mode = GET_BITS(val, 0, 3);
    ppu->Lcd.dispcnt.cg_mode = TEST_BIT(val, 3);
    ppu->Lcd.dispcnt.page = TEST_BIT(val, 4);
    ppu->Lcd.dispcnt.hblank_oam_access = TEST_BIT(val, 5);
    ppu->Lcd.dispcnt.oam_mapping_1d = TEST_BIT(val, 6);
    ppu->Lcd.dispcnt.forced_blank = TEST_BIT(val, 7);
    break;
  case DISPCNT + 1:
    ppu->Lcd.dispcnt.val = (ppu->Lcd.dispcnt.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 8; i++) {
      ppu->Lcd.dispcnt.enable[i] = TEST_BIT(val, i);
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
    ppu->Lcd.dispstat.vblank_irq = TEST_BIT(val, 3);
    ppu->Lcd.dispstat.hblank_irq = TEST_BIT(val, 4);
    ppu->Lcd.dispstat.vcounter_irq = TEST_BIT(val, 5);
    break;
  case DISPSTAT + 1:
    ppu->Lcd.dispstat.val = (ppu->Lcd.dispstat.val & 0x00FF) | (val << 8);
    ppu->Lcd.dispstat.vcount_setting = val;
    break;
  case BG0CNT:
  case BG1CNT:
  case BG2CNT:
  case BG3CNT: {
    int i = (addr - BG0CNT) / 2;
    ppu->Lcd.bgcnt[i].val = (ppu->Lcd.bgcnt[i].val & 0xFF00) | val;
    ppu->Lcd.bgcnt[i].priority = GET_BITS(val, 0, 2);
    ppu->Lcd.bgcnt[i].char_base_block = GET_BITS(val, 2, 2);
    ppu->Lcd.bgcnt[i].mosaic = TEST_BIT(val, 6);
    ppu->Lcd.bgcnt[i].colors = TEST_BIT(val, 7);
    break;
  }
  case BG0CNT + 1:
  case BG1CNT + 1:
  case BG2CNT + 1:
  case BG3CNT + 1: {
    int i = (addr - BG0CNT - 1) / 2;
    ppu->Lcd.bgcnt[i].val = (ppu->Lcd.bgcnt[i].val & 0x00FF) | (val << 8);
    ppu->Lcd.bgcnt[i].screen_base_block = GET_BITS(val, 0, 5);
    ppu->Lcd.bgcnt[i].aff_wrap = TEST_BIT(val, 5);
    ppu->Lcd.bgcnt[i].screen_size = GET_BITS(val, 6, 2);
    break;
  }
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
      ppu->Lcd.winin.win0[i] = TEST_BIT(val, i);
    }
    break;
  case WININ + 1:
    ppu->Lcd.winin.val = (ppu->Lcd.winin.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winin.win1[i] = TEST_BIT(val, i);
    }
    break;
  case WINOUT:
    ppu->Lcd.winout.val = (ppu->Lcd.winout.val & 0xFF00) | val;
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winout.win_out[i] = TEST_BIT(val, i);
    }
    break;
  case WINOUT + 1:
    ppu->Lcd.winout.val = (ppu->Lcd.winout.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.winout.win_obj[i] = TEST_BIT(val, i);
    }
    break;
  case MOSAIC:
    ppu->Lcd.mosaic.bg_h = GET_BITS(val, 0, 4);
    ppu->Lcd.mosaic.bg_v = GET_BITS(val, 4, 4);
    break;
  case MOSAIC + 1:
    ppu->Lcd.mosaic.obj_h = GET_BITS(val, 0, 4);
    ppu->Lcd.mosaic.obj_v = GET_BITS(val, 4, 4);
    break;
  case BLDCNT:
    ppu->Lcd.blendcnt.val = (ppu->Lcd.blendcnt.val & 0xFF00) | val;
    ppu->Lcd.blendcnt.effect = GET_BITS(val, 6, 2);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.blendcnt.targets[0][i] = TEST_BIT(val, i);
    }
    break;
  case BLDCNT + 1:
    ppu->Lcd.blendcnt.val = (ppu->Lcd.blendcnt.val & 0x00FF) | (val << 8);
    for (int i = 0; i < 6; i++) {
      ppu->Lcd.blendcnt.targets[1][i] = TEST_BIT(val, i);
    }
    break;
  case BLDALPHA:
    ppu->Lcd.eva = GET_BITS(val, 0, 5);
    break;
  case BLDALPHA + 1:
    ppu->Lcd.evb = GET_BITS(val, 0, 5);
    break;
  case BLDY:
    ppu->Lcd.evy = GET_BITS(val, 0, 5);
    break;
  case BLDY + 1:
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
    if (interrupt_pending(gba)) {
      scheduler_push_event(&gba->scheduler, EVENT_TYPE_IRQ, 0);
    }
    break;
  case IE + 1:
    int_mgr->ie = (int_mgr->ie & 0x00FF) | (val << 8);
    if (interrupt_pending(gba)) {
      scheduler_push_event(&gba->scheduler, EVENT_TYPE_IRQ, 0);
    }
    break;
  case IF:
    int_mgr->if_ &= ~val;
    break;
  case IF + 1:
    int_mgr->if_ &= ~(val << 8);
    break;
  case IME:
    int_mgr->ime = (int_mgr->ime & 0xFF00) | val;
    if (interrupt_pending(gba)) {
      scheduler_push_event(&gba->scheduler, EVENT_TYPE_IRQ, 0);
    }
    break;
  case IME + 1:
    int_mgr->ime = (int_mgr->ime & 0x00FF) | (val << 8);
    if (interrupt_pending(gba)) {
      scheduler_push_event(&gba->scheduler, EVENT_TYPE_IRQ, 0);
    }
    break;

  case HALTCNT:
    if (TEST_BIT(val, 7)) {
      io->power_state = POWER_STATE_STOPPED;
    } else {
      io->power_state = POWER_STATE_HALTED;
    }
    break;

  default:
    // printf("unhandled io write: %08X\n", addr);
    break;
  }
}

void io_write16(Gba *gba, u32 addr, u16 val) {
  Ppu *ppu = &gba->ppu;
  Apu *apu = &gba->apu;
  Io *io = &gba->io;
  Keypad *keypad = &gba->keypad;
  Dma *dma = &gba->dma;
  TimerManager *tmr_mgr = &gba->tmr_mgr;

  switch (addr) {
  // LCD
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

  // Sound
  case SOUNDBIAS:
    apu->soundbias = val;
    break;

  // DMA
  case DMA0SAD:
  case DMA1SAD:
  case DMA2SAD:
  case DMA3SAD: {
    int ch = (addr - DMA0SAD) / 12;
    dma->channels[ch].src_addr =
        (dma->channels[ch].src_addr & 0xFFFF0000) | val;
    break;
  }
  case DMA0SAD + 2:
  case DMA1SAD + 2:
  case DMA2SAD + 2:
  case DMA3SAD + 2: {
    int ch = (addr - (DMA0SAD + 2)) / 12;
    dma->channels[ch].src_addr =
        (dma->channels[ch].src_addr & 0x0000FFFF) | (val << 16);
    break;
  }
  case DMA0DAD:
  case DMA1DAD:
  case DMA2DAD:
  case DMA3DAD: {
    int ch = (addr - DMA0DAD) / 12;
    dma->channels[ch].dst_addr =
        (dma->channels[ch].dst_addr & 0xFFFF0000) | val;
    break;
  }
  case DMA0DAD + 2:
  case DMA1DAD + 2:
  case DMA2DAD + 2:
  case DMA3DAD + 2: {
    int ch = (addr - (DMA0DAD + 2)) / 12;
    dma->channels[ch].dst_addr =
        (dma->channels[ch].dst_addr & 0x0000FFFF) | (val << 16);
    break;
  }
  case DMA0CNT_L:
  case DMA1CNT_L:
  case DMA2CNT_L:
  case DMA3CNT_L: {
    int ch = (addr - DMA0CNT_L) / 12;
    dma->channels[ch].count = val;
    break;
  }
  case DMA0CNT_H:
  case DMA1CNT_H:
  case DMA2CNT_H:
  case DMA3CNT_H: {
    int ch = (addr - DMA0CNT_H) / 12;
    dma_control_write(gba, ch, val);
    break;
  }

  // Timer
  case TM0CNT_L:
  case TM1CNT_L:
  case TM2CNT_L:
  case TM3CNT_L: {
    int tmr = (addr - TM0CNT_L) / 4;
    tmr_mgr->timers[tmr].reload_count = val;
    break;
  }
  case TM0CNT_H:
  case TM1CNT_H:
  case TM2CNT_H:
  case TM3CNT_H: {
    int tmr = (addr - TM0CNT_H) / 4;
    timer_control_write(gba, tmr, val);
    break;
  }

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

void io_write32(Gba *gba, u32 addr, u32 val) {
  io_write16(gba, addr, val & 0xFFFF);
  io_write16(gba, addr + 2, (val >> 16) & 0xFFFF);
}
