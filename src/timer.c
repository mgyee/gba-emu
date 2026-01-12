#include "timer.h"
#include "gba.h"
#include "interrupt.h"
#include <string.h>

#define OVERFLOW 0x10000

void timer_init(TimerManager *tmr_mgr) {
  memset(tmr_mgr, 0, sizeof(TimerManager));
}

void timer_step(Gba *gba, int cycles) {
  TimerManager *tmr_mgr = &gba->tmr_mgr;

  int previous_overflows = 0;
  for (int i = 0; i < 4; i++) {
    Timer *timer = &tmr_mgr->timers[i];
    TimerControl *control = &timer->control;

    if (control->enable) {
      int increment;
      if (control->cascade) {
        increment = previous_overflows;
      } else {
        timer->cycle_count += cycles;
        increment = timer->cycle_count / control->freq;
        timer->cycle_count %= control->freq;
      }

      int period = OVERFLOW - timer->reload_count;
      int to_overflow = OVERFLOW - timer->count;

      if (increment >= to_overflow) {
        previous_overflows = 1 + (increment - to_overflow) / period;
        timer->count = timer->reload_count + (increment - to_overflow) % period;
        if (control->irq) {
          raise_interrupt(gba, INT_TIMER0 + i);
        }
      } else {
        timer->count += increment;
        previous_overflows = 0;
      }
    } else {
      // Timer disabled
      previous_overflows = 0;
    }
  }
}
