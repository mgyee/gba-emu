#include "dma.h"
#include "bus.h"
#include "common.h"
#include "gba.h"
#include "interrupt.h"
#include <assert.h>
#include <string.h>

void dma_init(Dma *dma) { memset(dma, 0, sizeof(Dma)); }

bool dma_active(Dma *dma) { return dma->active != 0; }

void dma_activate(Dma *dma, int ch) {
  dma->channels[ch].access = ACCESS_NONSEQ;
  dma->active |= BIT(ch);
}

void dma_on_vblank(Gba *gba) {
  Dma *dma = &gba->dma;
  for (int ch = 0; ch < 4; ch++) {
    DmaControl control = dma->channels[ch].control;
    if (control.enable && control.timing == TIMING_MODE_VBLANK) {
      dma_activate(dma, ch);
    }
  }
}

void dma_on_hblank(Gba *gba) {
  Dma *dma = &gba->dma;
  for (int ch = 0; ch < 4; ch++) {
    DmaControl control = dma->channels[ch].control;
    if (control.enable && control.timing == TIMING_MODE_HBLANK) {
      dma_activate(dma, ch);
    }
  }
}

void dma_transfer(Gba *gba, int ch) {
  Dma *dma = &gba->dma;
  DmaChannel *channel = &dma->channels[ch];
  u32 src = channel->internal_src_addr;
  u32 dst = channel->internal_dst_addr;
  Access access = channel->access;
  DmaControl *control = &channel->control;

  if (control->chunk_size == 4) {
    u32 data = bus_read32(gba, src, access);
    bus_write32(gba, dst, data, access);
  } else {
    u16 data = bus_read16(gba, src, access);
    bus_write16(gba, dst, data, access);
  }

  channel->access = ACCESS_SEQ;

  switch (control->src_adjustment) {
  case ADJUSTMENT_MODE_FIXED:
    break;
  case ADJUSTMENT_MODE_DECREMENT:
    channel->internal_src_addr -= control->chunk_size;
    break;
  case ADJUSTMENT_MODE_INCREMENT:
    channel->internal_src_addr += control->chunk_size;
    break;
  case ADJUSTMENT_MODE_RELOAD:
    assert(false);
  }

  switch (control->dst_adjustment) {
  case ADJUSTMENT_MODE_FIXED:
    break;
  case ADJUSTMENT_MODE_DECREMENT:
    channel->internal_dst_addr -= control->chunk_size;
    break;
  case ADJUSTMENT_MODE_INCREMENT:
  case ADJUSTMENT_MODE_RELOAD:
    channel->internal_dst_addr += control->chunk_size;
    break;
  }

  channel->internal_count -= 1;

  if (channel->internal_count == 0) {
    if (control->repeat) {
      if (control->dst_adjustment == ADJUSTMENT_MODE_RELOAD) {
        channel->internal_dst_addr = channel->dst_addr;
      }
      if (channel->count == 0) {
        if (ch == 3) {
          channel->internal_count = 0x10000;
        } else {
          channel->internal_count = 0x4000;
        }
      } else {
        channel->internal_count = channel->count;
      }
    } else {
      control->enable = false;
    }

    if (control->irq) {
      switch (ch) {
      case 0:
        raise_interrupt(gba, INT_DMA0);
        break;
      case 1:
        raise_interrupt(gba, INT_DMA1);
        break;
      case 2:
        raise_interrupt(gba, INT_DMA2);
        break;
      case 3:
        raise_interrupt(gba, INT_DMA3);
        break;
      }
    }

    dma->active &= ~BIT(ch);
  }
}

void dma_step(Gba *gba) {
  Dma *dma = &gba->dma;

  for (int ch = 0; ch < 4; ch++) {
    if (TEST_BIT(dma->active, ch)) {
      dma_transfer(gba, ch);
    }
  }
}
