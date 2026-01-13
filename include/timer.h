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

  uint start_time;

  TimerControl control;
} Timer;

typedef struct {
  Timer timers[4];
} TimerManager;

void timer_init(TimerManager *tmr_mgr);
void timer_step(Gba *gba, int cycles);

void timer_control_write(Gba *gba, int tmr, u16 val);

// void timer_update(Gba *gba, int tmr);
u16 timer_get_count(Gba *gba, int tmr);
void timer_overflow(Gba *gba, int tmr, uint lateness);
