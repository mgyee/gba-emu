#pragma once
#include "common.h"

typedef struct {
  u16 val;

  int freq;
  bool cascade;
  bool irq;
  bool enable;
} TimerControl;

typedef struct {
  u16 count;
  u16 reload_count;

  int cycle_count;

  TimerControl control;
} Timer;

typedef struct {
  Timer timers[4];
} TimerManager;

void timer_init(TimerManager *tmr_mgr);
void timer_step(Gba *gba, int cycles);
