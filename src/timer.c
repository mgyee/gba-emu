#include "timer.h"
#include "gba.h"
#include "interrupt.h"
#include "scheduler.h"
#include <string.h>

#define OVERFLOW 0x10000

void timer_init(TimerManager *tmr_mgr) {
  memset(tmr_mgr, 0, sizeof(TimerManager));
}

void timer_control_write(Gba *gba, int tmr, u16 val) {
  TimerManager *tmr_mgr = &gba->tmr_mgr;
  Timer *timer = &tmr_mgr->timers[tmr];
  TimerControl *control = &timer->control;

  bool was_enabled = control->enable;

  control->val = val;
  int freq = GET_BITS(val, 0, 2);
  switch (freq) {
  case 0:
    control->freq = 1;
    break;
  case 1:
    control->freq = 64;
    break;
  case 2:
    control->freq = 256;
    break;
  case 3:
    control->freq = 1024;
    break;
  }
  control->cascade = TEST_BIT(val, 2);
  control->irq = TEST_BIT(val, 6);
  control->enable = TEST_BIT(val, 7);

  scheduler_cancel_event(&gba->scheduler, EVENT_TYPE_TIMER_OVERFLOW,
                         (void *)(intptr_t)tmr);

  if (!was_enabled && control->enable) {
    timer->count = timer->reload_count;

    if (!control->cascade) {
      int time_to_overflow = (OVERFLOW - timer->count) * control->freq;
      scheduler_push_event_ctx(&gba->scheduler, EVENT_TYPE_TIMER_OVERFLOW,
                               time_to_overflow, (void *)(intptr_t)tmr);
    }
  }
}

u16 timer_get_count(Gba *gba, int tmr) {
  TimerManager *tmr_mgr = &gba->tmr_mgr;
  Timer *timer = &tmr_mgr->timers[tmr];
  TimerControl *control = &timer->control;

  if (!control->enable) {
    return timer->count;
  }

  if (control->cascade) {
    return timer->count;
  }

  uint elapsed_cycles = gba->scheduler.current_time - timer->start_time;
  uint increments = elapsed_cycles / control->freq;

  uint current_count = timer->count + increments;

  if (current_count >= OVERFLOW) {
    return timer->reload_count + (current_count - OVERFLOW);
  } else {
    return (u16)current_count;
  }
}

void timer_overflow(Gba *gba, int tmr, uint lateness) {
  TimerManager *tmr_mgr = &gba->tmr_mgr;
  Timer *timer = &tmr_mgr->timers[tmr];
  TimerControl *control = &timer->control;

  timer->count = timer->reload_count;

  if (control->irq) {
    raise_interrupt(gba, INT_TIMER0 + tmr);
  }

  timer->start_time = gba->scheduler.current_time - lateness;

  if (!control->cascade) {
    int time_to_overflow = (OVERFLOW - timer->count) * control->freq - lateness;
    scheduler_push_event_ctx(&gba->scheduler, EVENT_TYPE_TIMER_OVERFLOW,
                             time_to_overflow, (void *)(intptr_t)tmr);
  }

  if (tmr < 3) {
    Timer *next_timer = &tmr_mgr->timers[tmr + 1];
    if (next_timer->control.enable && next_timer->control.cascade) {
      next_timer->count++;
      if (next_timer->count == 0) {
        timer_overflow(gba, tmr + 1, lateness);
      }
    }
  }
}

// void timer_step(Gba *gba, int cycles) {
//   TimerManager *tmr_mgr = &gba->tmr_mgr;
//
//   int previous_overflows = 0;
//   for (int i = 0; i < 4; i++) {
//     Timer *timer = &tmr_mgr->timers[i];
//     TimerControl *control = &timer->control;
//
//     if (control->enable) {
//       int increment;
//       if (control->cascade) {
//         increment = previous_overflows;
//       } else {
//         timer->cycle_count += cycles;
//         increment = timer->cycle_count / control->freq;
//         timer->cycle_count %= control->freq;
//       }
//
//       int period = OVERFLOW - timer->reload_count;
//       int to_overflow = OVERFLOW - timer->count;
//
//       if (increment >= to_overflow) {
//         previous_overflows = 1 + (increment - to_overflow) / period;
//         timer->count = timer->reload_count + (increment - to_overflow) %
//         period; if (control->irq) {
//           raise_interrupt(gba, INT_TIMER0 + i);
//         }
//       } else {
//         timer->count += increment;
//         previous_overflows = 0;
//       }
//     } else {
//       // Timer disabled
//       previous_overflows = 0;
//     }
//   }
// }
