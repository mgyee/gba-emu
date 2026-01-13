#include "cpu.h"
#include "gba.h"
#include <string.h>

void cpu_init(Cpu *cpu) {
  memset(cpu, 0, sizeof(Cpu));
  arm_init_lut();
  thumb_init_lut();

  cpu->regs[13] = cpu->regs_fiq[5] = cpu->regs_abt[0] = cpu->regs_und[0] =
      0x03007F00;
  cpu->regs_svc[0] = cpu->regs_irq[0] = 0x03007FE0;

  cpu->regs[15] = 0x08000000;

  cpu->cpsr |= MODE_SYS | CPSR_I | CPSR_F;

  cpu->spsr = &cpu->cpsr;

  // cpu->regs[15] = 0x00000000;
  // cpu->cpsr |= MODE_SVC | CPSR_I | CPSR_F;
  // cpu->spsr = &cpu->cpsr;

  cpu->next_fetch_access = ACCESS_NONSEQ;
}

void cpu_set_mode(Cpu *cpu, u32 new_mode) {
  u32 old_mode = cpu->cpsr & 0x1F;
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
    // default:
    //   cpu->regs_usr[0] = cpu->regs[13];
    //   cpu->regs_usr[1] = cpu->regs[14];
    //   break;
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
    // default:
    //   cpu->regs[13] = cpu->regs_usr[0];
    //   cpu->regs[14] = cpu->regs_usr[1];
    //   cpu->spsr = &cpu->cpsr;
    //   break;
  }

  cpu->cpsr = (cpu->cpsr & ~0x1F) | new_mode;
}

int cpu_step(Gba *gba) {
  int cycles = 0;

#ifdef DEBUG
  printf("%08X: ", PC);
#endif
  if (CPSR & CPSR_T) {
    cycles += thumb_step(gba);
  } else {
    cycles += arm_step(gba);
  }
  return cycles;
}

bool check_cond(Cpu *cpu, u32 instr) {
  u8 cond = GET_BITS(instr, 28, 4);
  bool N = cpu->cpsr & CPSR_N;
  bool Z = cpu->cpsr & CPSR_Z;
  bool C = cpu->cpsr & CPSR_C;
  bool V = cpu->cpsr & CPSR_V;

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

ShiftRes LSL(Cpu *cpu, u32 val, u32 amt) {
  ShiftRes res;
  if (amt == 0) {
    res.value = val;
    res.carry = get_flag(cpu, CPSR_C);
  } else if (amt < 32) {
    res.value = val << amt;
    res.carry = (val >> (32 - amt)) & 1;
  } else if (amt == 32) {
    res.value = 0;
    res.carry = val & 1;
  } else {
    res.value = 0;
    res.carry = false;
  }
  return res;
}
ShiftRes LSR(Cpu *cpu, u32 val, u32 amt, bool imm) {
  ShiftRes res;
  if (amt == 0) {
    if (imm) {
      amt = 32;
    } else {
      res.value = val;
      res.carry = get_flag(cpu, CPSR_C);
      return res;
    }
  }

  if (amt < 32) {
    res.value = val >> amt;
    res.carry = (val >> (amt - 1)) & 1;
  } else if (amt == 32) {
    res.value = 0;
    res.carry = (val >> 31) & 1;
  } else {
    res.value = 0;
    res.carry = false;
  }
  return res;
}
ShiftRes ASR(Cpu *cpu, u32 val, u32 amt, bool imm) {
  ShiftRes res;
  if (amt == 0) {
    if (imm) {
      amt = 32;
    } else {
      res.value = val;
      res.carry = get_flag(cpu, CPSR_C);
      return res;
    }
  }

  if (amt < 32) {
    s32 sval = (s32)val;
    res.value = (u32)(sval >> amt);
    res.carry = (val >> (amt - 1)) & 1;
  } else {
    s32 sval = (s32)val;
    if (sval < 0) {
      res.value = 0xFFFFFFFF;
      res.carry = 1;
    } else {
      res.value = 0;
      res.carry = 0;
    }
  }
  return res;
}
ShiftRes ROR(Cpu *cpu, u32 val, u32 amt, bool imm) {
  ShiftRes res;
  if (amt == 0) {
    if (imm) {
      // RRX
      bool c = get_flag(cpu, CPSR_C);
      res.value = (val >> 1) | ((u32)c << 31);
      res.carry = val & 1;
      return res;
    } else {
      res.value = val;
      res.carry = get_flag(cpu, CPSR_C);
      return res;
    }
  }

  amt &= 31;
  if (amt == 0) {
    res.value = val;
    res.carry = (val >> 31) & 1;
  } else {
    res.value = (val >> amt) | (val << (32 - amt));
    res.carry = (val >> (amt - 1)) & 1;
  }
  return res;
}

ShiftRes barrel_shifter(Cpu *cpu, Shift shift, u32 val, u32 amt, bool imm) {
  switch (shift) {
  case SHIFT_LSL:
    return LSL(cpu, val, amt);
  case SHIFT_LSR:
    return LSR(cpu, val, amt, imm);
  case SHIFT_ASR:
    return ASR(cpu, val, amt, imm);
  case SHIFT_ROR:
    return ROR(cpu, val, amt, imm);
  }
}

bool get_flag(Cpu *cpu, u32 flag) { return (cpu->cpsr & flag) != 0; }

void set_flags(Cpu *cpu, u32 res, bool carry, bool overflow) {
  u32 flags = 0;

  flags |= (res & CPSR_N);
  flags |= (res == 0) << 30;
  flags |= carry << 29;
  flags |= overflow << 28;
  cpu->cpsr = (cpu->cpsr & ~(CPSR_N | CPSR_Z | CPSR_C | CPSR_V)) | flags;
  // CPSR = (CPSR & ~(CPSR_N | CPSR_Z | CPSR_C | CPSR_V)) | flags;
}

void set_flags_nzc(Cpu *cpu, u32 res, bool carry) {
  u32 flags = 0;
  flags |= (res & CPSR_N);
  flags |= (res == 0) << 30;
  flags |= carry << 29;
  cpu->cpsr = (cpu->cpsr & ~(CPSR_N | CPSR_Z | CPSR_C)) | flags;
  // CPSR = (CPSR & ~(CPSR_N | CPSR_Z | CPSR_C)) | flags;
}

void set_flags_nz(Cpu *cpu, u32 res) {
  u32 flags = 0;
  flags |= (res & CPSR_N);
  flags |= (res == 0) << 30;
  cpu->cpsr = (cpu->cpsr & ~(CPSR_N | CPSR_Z)) | flags;
  // CPSR = (CPSR & ~(CPSR_N | CPSR_Z)) | flags;
}
