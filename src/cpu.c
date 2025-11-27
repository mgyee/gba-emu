#include "cpu.h"
#include <string.h>

void cpu_init(CPU *cpu) {
  memset(cpu->regs, 0, sizeof(cpu->regs));
  PC = 0x08000000;
  cpu->cpsr = 0x1F;
}

void cpu_step(CPU *cpu, Bus *bus) {
  if (cpu->cpsr & CPSR_T) {
    thumb_step(cpu, bus);
  } else {
    arm_step(cpu, bus);
  }
}
