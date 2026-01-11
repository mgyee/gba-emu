#pragma once
#include "common.h"

typedef enum {
  INT_VBLANK,
  INT_HBLANK,
  INT_VCOUNT,
  INT_TIMER0,
  INT_TIMER1,
  INT_TIMER2,
  INT_TIMER3,
  INT_SERIAL,
  INT_DMA0,
  INT_DMA1,
  INT_DMA2,
  INT_DMA3,
  INT_KEYPAD,
  INT_GAMEPAK
} InterruptType;

typedef struct {
  u16 ie;
  u16 if_;
  u16 ime;
} InterruptManager;

void interrupt_init(InterruptManager *int_mgr);
void raise_interrupt(Gba *gba, InterruptType type);
bool interrupt_pending(Gba *gba);
void handle_interrupts(Gba *gba);
