#include "interrupt.h"
#include "gba.h"

void interrupt_init(InterruptManager *int_mgr) {
  int_mgr->ie = 0;
  int_mgr->if_ = 0;
  int_mgr->ime = 0;
}

void raise_interrupt(Gba *gba, InterruptType type) {
  gba->int_mgr.if_ |= (1 << type);
}

void handle_interrupts(Gba *gba) {
  InterruptManager *int_mgr = &gba->int_mgr;

  if ((int_mgr->ime & 1) == 0) {
    return;
  }

  u16 pending = int_mgr->ie & int_mgr->if_;
  if (pending == 0) {
    return;
  }

  for (int i = 0; i < 14; i++) {
    if (pending & (1 << i)) {
      int_mgr->if_ &= ~(1 << i);

      gba->cpu.spsr_svc = CPSR;
      cpu_set_mode(&gba->cpu, MODE_IRQ);
      CPSR |= CPSR_I;
      LR = PC - 8;
      PC = 0x18;

      if (CPSR & CPSR_T) {
        arm_fetch(gba);
      } else {
        thumb_fetch(gba);
      }

      break;
    }
  }
}
