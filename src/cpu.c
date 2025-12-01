#include "cpu.h"
#include <string.h>

void cpu_init(CPU *cpu) {
  memset(cpu, 0, sizeof(CPU));
  arm_lut_init();
}

void cpu_set_mode(CPU *cpu, u32 new_mode) {
  u32 old_mode = CPSR & 0x1F;
  if (old_mode == new_mode)
    return;

  switch (old_mode) {
  case MODE_USR:
  case MODE_SYS:
    cpu->regs_usr[0] = cpu->regs[13];
    cpu->regs_usr[1] = cpu->regs[14];
    break;
  case MODE_FIQ:
    cpu->regs_fiq[0] = cpu->regs[8];
    cpu->regs_fiq[1] = cpu->regs[9];
    cpu->regs_fiq[2] = cpu->regs[10];
    cpu->regs_fiq[3] = cpu->regs[11];
    cpu->regs_fiq[4] = cpu->regs[12];
    cpu->regs_fiq[5] = cpu->regs[13];
    cpu->regs_fiq[6] = cpu->regs[14];
    break;
  case MODE_SVC:
    cpu->regs_svc[0] = cpu->regs[13];
    cpu->regs_svc[1] = cpu->regs[14];
    break;
  case MODE_ABT:
    cpu->regs_abt[0] = cpu->regs[13];
    cpu->regs_abt[1] = cpu->regs[14];
    break;
  case MODE_IRQ:
    cpu->regs_irq[0] = cpu->regs[13];
    cpu->regs_irq[1] = cpu->regs[14];
    break;
  case MODE_UND:
    cpu->regs_und[0] = cpu->regs[13];
    cpu->regs_und[1] = cpu->regs[14];
    break;
  default:
    cpu->regs_usr[0] = cpu->regs[13];
    cpu->regs_usr[1] = cpu->regs[14];
    break;
  }

  if (old_mode == MODE_FIQ && new_mode != MODE_FIQ) {
    cpu->regs[8] = cpu->regs_usr_hi[0];
    cpu->regs[9] = cpu->regs_usr_hi[1];
    cpu->regs[10] = cpu->regs_usr_hi[2];
    cpu->regs[11] = cpu->regs_usr_hi[3];
    cpu->regs[12] = cpu->regs_usr_hi[4];
  } else if (old_mode != MODE_FIQ && new_mode == MODE_FIQ) {
    cpu->regs_usr_hi[0] = cpu->regs[8];
    cpu->regs_usr_hi[1] = cpu->regs[9];
    cpu->regs_usr_hi[2] = cpu->regs[10];
    cpu->regs_usr_hi[3] = cpu->regs[11];
    cpu->regs_usr_hi[4] = cpu->regs[12];
  }

  switch (new_mode) {
  case MODE_USR:
  case MODE_SYS:
    cpu->regs[13] = cpu->regs_usr[0];
    cpu->regs[14] = cpu->regs_usr[1];
    cpu->spsr = &cpu->cpsr;
    break;
  case MODE_FIQ:
    cpu->regs[8] = cpu->regs_fiq[0];
    cpu->regs[9] = cpu->regs_fiq[1];
    cpu->regs[10] = cpu->regs_fiq[2];
    cpu->regs[11] = cpu->regs_fiq[3];
    cpu->regs[12] = cpu->regs_fiq[4];
    cpu->regs[13] = cpu->regs_fiq[5];
    cpu->regs[14] = cpu->regs_fiq[6];
    cpu->spsr = &cpu->spsr_fiq;
    break;
  case MODE_SVC:
    cpu->regs[13] = cpu->regs_svc[0];
    cpu->regs[14] = cpu->regs_svc[1];
    cpu->spsr = &cpu->spsr_svc;
    break;
  case MODE_ABT:
    cpu->regs[13] = cpu->regs_abt[0];
    cpu->regs[14] = cpu->regs_abt[1];
    cpu->spsr = &cpu->spsr_abt;
    break;
  case MODE_IRQ:
    cpu->regs[13] = cpu->regs_irq[0];
    cpu->regs[14] = cpu->regs_irq[1];
    cpu->spsr = &cpu->spsr_irq;
    break;
  case MODE_UND:
    cpu->regs[13] = cpu->regs_und[0];
    cpu->regs[14] = cpu->regs_und[1];
    cpu->spsr = &cpu->spsr_und;
    break;
  }

  cpu->cpsr = (cpu->cpsr & ~0x1F) | new_mode;
}

int cpu_step(CPU *cpu, Bus *bus) {
  int cycles = 0;
  bus->cycle_count = 0;
  if (cpu->cpsr & CPSR_T) {
    // printf("[CPU] Thumb Mode Step\n");
    cycles += thumb_step(cpu, bus);
  } else {
    // printf("[CPU] ARM Mode Step\n");
    cycles += arm_step(cpu, bus);
  }
  cycles += bus->cycle_count;
  return cycles;
}

bool check_cond(CPU *cpu, u32 instr) {
  u8 cond = GET_BITS(instr, 28, 4);
  bool N = (cpu->cpsr & CPSR_N);
  bool Z = (cpu->cpsr & CPSR_Z);
  bool C = (cpu->cpsr & CPSR_C);
  bool V = (cpu->cpsr & CPSR_V);

  switch (cond) {
  case 0x0: // EQ
    return Z;
  case 0x1: // NE
    return !Z;
  case 0x2: // CS / HS
    return C;
  case 0x3: // CC / LO
    return !C;
  case 0x4: // MI
    return N;
  case 0x5: // PL
    return !N;
  case 0x6: // VS
    return V;
  case 0x7: // VC
    return !V;
  case 0x8: // HI
    return C && !Z;
  case 0x9: // LS
    return !C || Z;
  case 0xA: // GE
    return N == V;
  case 0xB: // LT
    return N != V;
  case 0xC: // GT
    return !Z && (N == V);
  case 0xD: // LE
    return Z || (N != V);
  case 0xE: // AL
    return true;
  default:
    return false;
  }
}
