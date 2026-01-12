#pragma once
#include "bus.h"
#include "common.h"

typedef enum {
  ADJUSTMENT_MODE_INCREMENT,
  ADJUSTMENT_MODE_DECREMENT,
  ADJUSTMENT_MODE_FIXED,
  ADJUSTMENT_MODE_RELOAD
} AdjustmentMode;

typedef enum {
  TIMING_MODE_NOW,
  TIMING_MODE_VBLANK,
  TIMING_MODE_HBLANK,
  TIMING_MODE_REFRESH
} TimingMode;

typedef struct {
  u16 val;

  AdjustmentMode dst_adjustment;
  AdjustmentMode src_adjustment;

  bool repeat;

  int chunk_size;

  TimingMode timing;

  bool irq;
  bool enable;
} DmaControl;

typedef struct {
  u32 src_addr;
  u32 dst_addr;
  u16 count;

  u32 internal_src_addr;
  u32 internal_dst_addr;
  u32 internal_count;

  Access access;

  DmaControl control;

} DmaChannel;

typedef struct {
  DmaChannel channels[4];
  int active;
} Dma;

void dma_init(Dma *dma);
bool dma_active(Dma *dma);
void dma_step(Gba *gba);
void dma_activate(Dma *dma, int ch);

void dma_on_vblank(Gba *gba);
void dma_on_hblank(Gba *gba);
