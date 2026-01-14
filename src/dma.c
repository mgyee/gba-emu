#include "dma.h"
#include "bus.h"
#include "common.h"
#include "gba.h"
#include "interrupt.h"
#include <assert.h>
#include <string.h>

void dma_init(Dma *dma) { memset(dma, 0, sizeof(Dma)); }

// bool dma_active(Dma *dma) { return dma->active != 0; }

void dma_activate(Dma *dma, int ch) {
  dma->channels[ch].access = ACCESS_NONSEQ;
  // dma->active |= BIT(ch);
}

void dma_on_vblank(Gba *gba) {
  Dma *dma = &gba->dma;
  for (int ch = 0; ch < 4; ch++) {
    DmaControl control = dma->channels[ch].control;
    if (control.enable && control.timing == TIMING_MODE_VBLANK) {
      // dma_activate(dma, ch);
      dma->channels[ch].access = ACCESS_NONSEQ;
      scheduler_push_event_ctx(&gba->scheduler, EVENT_TYPE_DMA_ACTIVATE, 0,
                               (void *)(intptr_t)ch);
    }
  }
}

void dma_on_hblank(Gba *gba) {
  Dma *dma = &gba->dma;
  for (int ch = 0; ch < 4; ch++) {
    DmaControl control = dma->channels[ch].control;
    if (control.enable && control.timing == TIMING_MODE_HBLANK) {
      // dma_activate(dma, ch);
      dma->channels[ch].access = ACCESS_NONSEQ;
      scheduler_push_event_ctx(&gba->scheduler, EVENT_TYPE_DMA_ACTIVATE, 0,
                               (void *)(intptr_t)ch);
    }
  }
}

void dma_transfer(Gba *gba, int ch) {
  Dma *dma = &gba->dma;
  DmaChannel *channel = &dma->channels[ch];
  // Access access = channel->access;
  DmaControl *control = &channel->control;

  Access access = ACCESS_NONSEQ;

  while (channel->internal_count > 0) {
    u32 src = channel->internal_src_addr;
    u32 dst = channel->internal_dst_addr;
    if (control->chunk_size == 4) {
      u32 data = bus_read32(gba, src & ~3, access);
      bus_write32(gba, dst & ~3, data, access);
    } else {
      u16 data = bus_read16(gba, src & ~1, access);
      bus_write16(gba, dst & ~1, data, access);
    }

    // channel->access = ACCESS_SEQ;
    access = ACCESS_SEQ;

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

    channel->internal_count--;
  }

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
    raise_interrupt(gba, INT_DMA0 + ch);
  }

  // dma->active &= ~BIT(ch);
}

void dma_control_write(Gba *gba, int ch, u16 val) {
  Dma *dma = &gba->dma;
  DmaControl *control = &dma->channels[ch].control;

  bool was_enabled = control->enable;

  control->val = val;
  control->dst_adjustment = GET_BITS(val, 5, 2);
  control->src_adjustment = GET_BITS(val, 7, 2);
  control->repeat = TEST_BIT(val, 9);
  control->chunk_size = TEST_BIT(val, 10) ? 4 : 2;
  control->timing = GET_BITS(val, 12, 2);
  control->irq = TEST_BIT(val, 14);
  control->enable = TEST_BIT(val, 15);

  if (!was_enabled && control->enable) {
    DmaChannel *channel = &dma->channels[ch];
    channel->internal_src_addr = channel->src_addr;
    channel->internal_dst_addr = channel->dst_addr;
    if (channel->count == 0) {
      if (ch == 3) {
        channel->internal_count = 0x10000;
      } else {
        channel->internal_count = 0x4000;
      }
    } else {
      channel->internal_count = channel->count;
    }

    if (control->timing == TIMING_MODE_NOW) {
      // dma_activate(dma, ch);
      dma->channels[ch].access = ACCESS_NONSEQ;
      scheduler_push_event_ctx(&gba->scheduler, EVENT_TYPE_DMA_ACTIVATE, 2,
                               (void *)(intptr_t)ch);
    }
  }
}

// void dma_step(Gba *gba) {
//   Dma *dma = &gba->dma;
//
//   for (int ch = 0; ch < 4; ch++) {
//     if (TEST_BIT(dma->active, ch)) {
//       dma_transfer(gba, ch);
//     }
//   }
// }
