#include "interrupt.h"
#include "gba.h"

void interrupt_init(InterruptManager *int_mgr) {
  int_mgr->ie = 0;
  int_mgr->if_ = 0;
  int_mgr->ime = 0;
}

inline void raise_interrupt(Gba *gba, InterruptType type) {
  gba->int_mgr.if_ |= (1 << type);

  if (gba->int_mgr.ie & (1 << type)) {
    scheduler_push_event(&gba->scheduler, EVENT_TYPE_IRQ, 0);
  }
}

inline bool interrupt_pending(Gba *gba) {
  InterruptManager *int_mgr = &gba->int_mgr;
  return (int_mgr->ime & 1) && (int_mgr->ie & int_mgr->if_);
}

void handle_interrupts(Gba *gba) {

  if ((gba->cpu.cpsr & CPSR_I) == CPSR_I || !interrupt_pending(gba)) {
    return;
  }

  if (gba->io.power_state == POWER_STATE_HALTED) {
    gba->io.power_state = POWER_STATE_NORMAL;
  }

  gba->cpu.spsr_irq = CPSR;
  cpu_set_mode(&gba->cpu, MODE_IRQ);
  if (CPSR & CPSR_T) {
    LR = PC;
  } else {
    LR = PC - 4;
  }
  CPSR &= ~CPSR_T;
  CPSR |= CPSR_I;
  PC = 0x18;

  arm_fetch(gba);
}
