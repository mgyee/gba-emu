#include "cpu.h"
#include <stdio.h>

typedef int (*ArmInstr)(CPU *cpu, Bus *bus, u32 instr);

static ArmInstr arm_lut[4096];

static inline u32 decode_index(u32 instr) {
  u32 high = (instr >> 16) & 0xFF0;
  u32 low = (instr >> 4) & 0x00F;
  return high | low;
}

u32 arm_fetch_next(CPU *cpu, Bus *bus) {
  u32 instr = cpu->pipeline[0];
  cpu->pipeline[0] = cpu->pipeline[1];
  cpu->pipeline[1] = bus_read32(bus, PC, ACCESS_SEQ | ACCESS_CODE);
  PC += 4;
  return instr;
}

void arm_fetch(CPU *cpu, Bus *bus) {
  cpu->pipeline[0] = bus_read32(bus, PC, ACCESS_NONSEQ | ACCESS_CODE);
  cpu->pipeline[1] = bus_read32(bus, PC + 4, ACCESS_SEQ | ACCESS_CODE);
  PC += 8;
}

int arm_step(CPU *cpu, Bus *bus) {
  u32 instr = arm_fetch_next(cpu, bus);

  if (!check_cond(cpu, instr)) {
    printf("[ARM] Condition not met, skipping instruction: %08X\n", instr);
    return 0;
  }
  u32 index = decode_index(instr);
  printf("[ARM] Executing Instr: %08X\n", instr);
  if (instr == 0x0355001E) {
    printf("Breakpoint hit\n");
  }
  return arm_lut[index](cpu, bus, instr);
}

typedef enum { SHIFT_LSL, SHIFT_LSR, SHIFT_ASR, SHIFT_ROR } Shift;
typedef struct {
  u32 value;
  bool carry;
} ShiftRes;

ShiftRes LSL(CPU *cpu, u32 val, u32 amt) {
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
ShiftRes LSR(CPU *cpu, u32 val, u32 amt, bool imm) {
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
ShiftRes ASR(CPU *cpu, u32 val, u32 amt, bool imm) {
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
    i32 sval = (i32)val;
    res.value = (u32)(sval >> amt);
    res.carry = (val >> (amt - 1)) & 1;
  } else {
    i32 sval = (i32)val;
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
ShiftRes ROR(CPU *cpu, u32 val, u32 amt, bool imm) {
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

ShiftRes barrel_shifter(CPU *cpu, Shift shift, u32 val, u32 amt, bool imm) {
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

typedef enum {
  ALU_AND,
  ALU_EOR,
  ALU_SUB,
  ALU_RSB,
  ALU_ADD,
  ALU_ADC,
  ALU_SBC,
  ALU_RSC,
  ALU_TST,
  ALU_TEQ,
  ALU_CMP,
  ALU_CMN,
  ALU_ORR,
  ALU_MOV,
  ALU_BIC,
  ALU_MVN
} ALUOp;

static int arm_undefined(CPU *cpu, Bus *bus, u32 instr) {
  (void)cpu;
  (void)bus;
  printf("[ARM] Undefined instruction: %08X\n", instr);
  return 0;
}

// MUL, MLA
int arm_mul(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] MUL/MLA: %08X\n", instr);
  bool acc = TEST_BIT(instr, 21);
  bool s = TEST_BIT(instr, 20);
  u8 rd = GET_BITS(instr, 16, 4);
  u8 rn = GET_BITS(instr, 12, 4);
  u8 rs = GET_BITS(instr, 8, 4);
  u8 rm = GET_BITS(instr, 0, 4);

  u8 m_cycles = 1;
  u32 tmp = rs ^ ((i32)rs >> 31);
  m_cycles += (tmp > 0xff);
  m_cycles += (tmp > 0xffff);
  m_cycles += (tmp > 0xffffff);

  u32 res = REG(rm) * REG(rs);
  if (acc) {
    res += REG(rn);
    m_cycles += 1;
  }
  REG(rd) = res;

  if (rd == 15) {
    arm_fetch(cpu, bus);
  }

  if (s) {
    u32 flags = 0;
    if (res == 0) {
      flags |= CPSR_Z;
    }
    flags |= (res & CPSR_N);
    CPSR = (CPSR & ~(CPSR_N | CPSR_Z)) | flags;
  }

  return m_cycles;
}
// MULL, MLAL
int arm_mull(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] MULL/MLAL: %08X\n", instr);
  bool sign = TEST_BIT(instr, 22);
  bool acc = TEST_BIT(instr, 21);
  bool s = TEST_BIT(instr, 20);
  u8 rdhi = GET_BITS(instr, 16, 4);
  u8 rdlo = GET_BITS(instr, 12, 4);
  u8 rs = GET_BITS(instr, 8, 4);
  u8 rm = GET_BITS(instr, 0, 4);

  u8 m_cycles = 2;
  u32 tmp = rs ^ ((i32)rs >> 31);
  m_cycles += (tmp > 0xff);
  m_cycles += (tmp > 0xffff);
  m_cycles += (tmp > 0xffffff);

  if (sign) {
    // SMULL, SMLAL
    i64 res = (i64)(i32)REG(rm) * (i64)(i32)REG(rs);
    if (acc) {
      res += ((i64)REG(rdhi) << 32) + (i64)REG(rdlo);
    }

    REG(rdlo) = res & 0xFFFFFFFF;
    REG(rdhi) = res >> 32;

    if (rdhi == 15 || rdlo == 15) {
      arm_fetch(cpu, bus);
    }

    if (s) {
      u32 flags = 0;
      if (res == 0) {
        flags |= CPSR_Z;
      }
      flags |= (((u64)res >> 32) & CPSR_N);
      CPSR = (CPSR & ~(CPSR_N | CPSR_Z)) | flags;
    }
  } else {
    // UMULL, UMLAL
    u64 res = (u64)REG(rm) * (u64)REG(rs);
    if (acc) {
      res += ((u64)REG(rdhi) << 32) + (u64)REG(rdlo);
    }

    REG(rdlo) = res & 0xFFFFFFFF;
    REG(rdhi) = res >> 32;

    if (rdhi == 15 || rdlo == 15) {
      arm_fetch(cpu, bus);
    }

    if (s) {
      u32 flags = 0;
      if (res == 0) {
        flags |= CPSR_Z;
      }
      flags |= ((res >> 32) & CPSR_N);
      CPSR = (CPSR & ~(CPSR_N | CPSR_Z)) | flags;
    }
  }

  return m_cycles;
}

int arm_swp(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] SWP: %08X\n", instr);
  bool b = TEST_BIT(instr, 22);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 rm = GET_BITS(instr, 0, 4);

  u32 val;

  if (b) {
    // byte
    val = bus_read8(bus, REG(rn), ACCESS_NONSEQ);
    bus_write8(bus, REG(rn), REG(rm) & 0xFF, ACCESS_NONSEQ);
  } else {
    // word
    u32 addr = REG(rn);
    val = bus_read32(bus, addr, ACCESS_NONSEQ);
    u32 rot = (addr & 3) * 8;
    if (rot) {
      val = (val >> rot) | (val << (32 - rot));
    }
    bus_write32(bus, addr, REG(rm), ACCESS_NONSEQ);
  }

  REG(rd) = val;

  if (rd == 15) {
    arm_fetch(cpu, bus);
  }

  return 1;
}

int arm_ldrh_strh(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] LDRH/STRH: %08X\n", instr);

  bool p = TEST_BIT(instr, 24);
  bool u = TEST_BIT(instr, 23);
  bool i = TEST_BIT(instr, 22);
  bool w = TEST_BIT(instr, 21);
  bool l = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  u32 offset = 0;

  if (i) {
    // Immediate offset
    u8 offset_high = GET_BITS(instr, 8, 4);
    u8 offset_low = GET_BITS(instr, 0, 4);
    offset = (offset_high << 4) | offset_low;
  } else {
    // Register offset
    u8 rm = GET_BITS(instr, 0, 4);
    offset = REG(rm);
    if (rm == 15) {
      offset -= 4;
    }
  }

  if (!u) {
    offset = -offset;
  }

  bool wb = !p || w;

  u32 addr = rn_val + (p ? offset : 0);

  int cycles = 0;

  if (l) {
    // LDRH
    u16 val;
    if (addr & 1) {
      val = bus_read16(bus, addr, ACCESS_NONSEQ);
      ShiftRes sh_res = barrel_shifter(cpu, SHIFT_ROR, val, 8, true);
      REG(rd) = sh_res.value;
    } else {
      REG(rd) = bus_read16(bus, addr, ACCESS_NONSEQ);
    }
    if (rd == 15) {
      arm_fetch(cpu, bus);
    }
    cycles = 1;
  } else {
    // STRH
    bus_write16(bus, addr, REG(rd), ACCESS_NONSEQ);
    bus->last_access = ACCESS_NONSEQ;
  }

  if (wb && (!l || rd != rn)) {
    REG(rn) += offset + ((rn == 15) << 3);
  }

  return cycles;
}

int arm_ldrsb_ldrsh(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] LDRSB/LDRSH: %08X\n", instr);

  bool p = TEST_BIT(instr, 24);
  bool u = TEST_BIT(instr, 23);
  bool i = TEST_BIT(instr, 22);
  bool w = TEST_BIT(instr, 21);
  bool h = TEST_BIT(instr, 5);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  u32 offset = 0;

  if (i) {
    // Immediate offset
    u8 offset_high = GET_BITS(instr, 8, 4);
    u8 offset_low = GET_BITS(instr, 0, 4);
    offset = (offset_high << 4) | offset_low;
  } else {
    // Register offset
    u8 rm = GET_BITS(instr, 0, 4);
    offset = REG(rm);
    if (rm == 15) {
      offset -= 4;
    }
  }

  if (!u) {
    offset = -offset;
  }

  bool wb = !p || w;

  u32 addr = rn_val + (p ? offset : 0);

  u32 val;

  if (h) {
    // Halfword
    if (addr & 1) {
      val = bus_read16(bus, addr, ACCESS_NONSEQ) >> 8;
      if (val & 0x80) {
        val |= 0xFFFFFF00;
      }
    } else {
      val = bus_read16(bus, addr, ACCESS_NONSEQ);
      if (val & 0x8000) {
        val |= 0xFFFF0000;
      }
    }
  } else {
    // Byte
    val = bus_read8(bus, addr, ACCESS_NONSEQ);
    if (val & 0x80) {
      val |= 0xFFFFFF00;
    }
  }

  REG(rd) = val;
  if (rd == 15) {
    arm_fetch(cpu, bus);
  }

  if (wb && (rd != rn)) {
    REG(rn) += offset + ((rn == 15) << 3);
  }

  return 1;
}

int arm_mrs(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] MRS: %08X\n", instr);
  (void)bus;
  bool r = TEST_BIT(instr, 22);
  u8 rd = GET_BITS(instr, 12, 4);

  if (r) {
    // SPSR
    REG(rd) = SPSR;
  } else {
    // CPSR
    REG(rd) = CPSR;
  }

  if (rd == 15) {
    REG(15) += 4;
  }

  return 0;
}

int arm_msr_reg(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] MSR (reg): %08X\n", instr);
}

int arm_msr_imm(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] MSR (imm): %08X\n", instr);
}

int arm_bx(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] BX: %08X\n", instr);
  u8 rn = GET_BITS(instr, 0, 4);
  u32 target = REG(rn);
  if (rn == 15) {
    target -= 4;
  }
  bool thumb = target & 1;

  if (thumb) {
    CPSR |= CPSR_T;
    PC = target;
    thumb_fetch(cpu, bus);
  } else {
    CPSR &= ~CPSR_T;
    PC = target;
    arm_fetch(cpu, bus);
  }

  return 0;
}

static inline void arm_do_dproc(CPU *cpu, Bus *bus, ALUOp opcode, u32 op1,
                                u32 op2, u8 rd, bool s, bool carry) {
  u32 res = 0;
  bool overflow = get_flag(cpu, CPSR_V);
  bool arithmetic = false;

  bool r15_dst = (rd == 15);

  switch (opcode) {
  case ALU_AND:
    res = op1 & op2;
    break;
  case ALU_EOR:
    res = op1 ^ op2;
    break;
  case ALU_SUB: {
    u64 tmp = (u64)op1 - op2;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_RSB: {
    u64 tmp = (u64)op2 - op1;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op2 ^ op1) & (op2 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_ADD: {
    u64 tmp = (u64)op1 + op2;
    res = (u32)tmp;
    carry = (tmp >> 32) & 1;
    overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_ADC: {
    u32 c = get_flag(cpu, CPSR_C);
    u64 tmp = (u64)op1 + op2 + c;
    res = (u32)tmp;
    carry = (tmp >> 32) & 1;
    overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_SBC: {
    u32 c = get_flag(cpu, CPSR_C);
    u64 tmp = (u64)op1 - op2 - (1 - c);
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_RSC: {
    u32 c = get_flag(cpu, CPSR_C);
    u64 tmp = (u64)op2 - op1 - (1 - c);
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op2 ^ op1) & (op2 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_TST:
    res = op1 & op2;
    break;
  case ALU_TEQ:
    res = op1 ^ op2;
    break;
  case ALU_CMP: {
    u64 tmp = (u64)op1 - op2;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_CMN: {
    u64 tmp = (u64)op1 + op2;
    res = (u32)tmp;
    carry = (tmp >> 32) & 1;
    overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
    arithmetic = true;
    break;
  }
  case ALU_ORR:
    res = op1 | op2;
    break;
  case ALU_MOV:
    res = op2;
    break;
  case ALU_BIC:
    res = op1 & ~op2;
    break;
  case ALU_MVN:
    res = ~op2;
    break;
  }

  if (opcode != ALU_TST && opcode != ALU_TEQ && opcode != ALU_CMP &&
      opcode != ALU_CMN) {
    REG(rd) = res;
    if (r15_dst) {
      arm_fetch(cpu, bus);
    }
  }

  if (s) {
    if (arithmetic) {
      set_flags(cpu, res, carry, overflow);
    } else {
      set_flags_nzc(cpu, res, carry);
    }
    if (r15_dst) {
      u32 spsr = SPSR;
      cpu_set_mode(cpu, SPSR & 0x1F);
      CPSR = spsr;
    }
  }
}

int arm_data_proc_imm_shift(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] Data Proc (imm shift): %08X\n", instr);
  ALUOp op = (ALUOp)GET_BITS(instr, 21, 4);
  bool s = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 shift_amt = GET_BITS(instr, 7, 5);
  Shift shift_type = (Shift)GET_BITS(instr, 5, 2);
  u8 rm = GET_BITS(instr, 0, 4);

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  u32 rm_val = REG(rm);
  if (rm == 15) {
    rm_val -= 4;
  }

  ShiftRes sh_res = barrel_shifter(cpu, shift_type, rm_val, shift_amt, true);

  arm_do_dproc(cpu, bus, op, rn_val, sh_res.value, rd, s, sh_res.carry);
  return 0;
}

int arm_data_proc_reg_shift(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] Data Proc (reg shift): %08X\n", instr);
  ALUOp op = (ALUOp)GET_BITS(instr, 21, 4);
  bool s = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 rs = GET_BITS(instr, 8, 4);
  Shift shift_type = (Shift)GET_BITS(instr, 5, 2);
  u8 rm = GET_BITS(instr, 0, 4);

  u32 rn_val = REG(rn);

  u32 rs_val = REG(rs);
  if (rs == 15) {
    rs_val -= 4;
  }

  u32 rm_val = REG(rm);

  ShiftRes sh_res =
      barrel_shifter(cpu, shift_type, rm_val, rs_val & 0xFF, false);

  arm_do_dproc(cpu, bus, op, rn_val, sh_res.value, rd, s, sh_res.carry);
  return 1;
}

int arm_data_proc_imm(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] Data Proc (imm val): %08X\n", instr);
  ALUOp op = (ALUOp)GET_BITS(instr, 21, 4);
  bool s = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 shift_amt = GET_BITS(instr, 8, 4) * 2;
  u8 imm = GET_BITS(instr, 0, 8);

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  ShiftRes sh_res = barrel_shifter(cpu, SHIFT_ROR, imm, shift_amt, false);

  arm_do_dproc(cpu, bus, op, rn_val, sh_res.value, rd, s, sh_res.carry);
  return 0;
}

int arm_ldr_str_imm(CPU *cpu, Bus *bus, u32 instr) {
  NOT_YET_IMPLEMENTED("LDR/STR (immediate offset)");
}

int arm_ldr_str_reg(CPU *cpu, Bus *bus, u32 instr) {
  NOT_YET_IMPLEMENTED("LDR/STR (register offset)");
}

int arm_ldm_stm(CPU *cpu, Bus *bus, u32 instr) {
  NOT_YET_IMPLEMENTED("LDM/STM");
}

int arm_branch(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] B/BL: %08X\n", instr);

  bool link = TEST_BIT(instr, 24);
  i32 offset = GET_BITS(instr, 0, 24);

  if (offset & 0x800000) {
    offset |= 0xFF000000;
  }
  offset <<= 2;

  if (link) {
    REG(14) = PC - 8;
  }

  PC += offset - 4;

  arm_fetch(cpu, bus);

  return 0;
}

int arm_stc_ldc(CPU *cpu, Bus *bus, u32 instr) {
  NOT_YET_IMPLEMENTED("STC/LDC");
}

int arm_cdp(CPU *cpu, Bus *bus, u32 instr) {
  NOT_YET_IMPLEMENTED("CDP");
  (void)cpu;
  (void)bus;
  (void)instr;
}

int arm_mcr_mrc(CPU *cpu, Bus *bus, u32 instr) {
  NOT_YET_IMPLEMENTED("MCR/MRC");
  (void)cpu;
  (void)bus;
  (void)instr;
}

int arm_swi(CPU *cpu, Bus *bus, u32 instr) {
  printf("[ARM] SWI: %08X\n", instr);
  cpu->spsr_svc = CPSR;
  cpu_set_mode(cpu, MODE_SVC);
  REG(14) = PC - 8;
  PC = 0x8;
  CPSR |= CPSR_I;
  arm_fetch(cpu, bus);
  return 0;
}

void arm_lut_init() {
  for (int i = 0; i < 4096; i++) {
    if ((i & 0b111111001111) == 0b000000001001) {
      arm_lut[i] = arm_mul; // MUL, MLA
    } else if ((i & 0b111110001111) == 0b000010001001) {
      arm_lut[i] = arm_mull; // MULL, MLAL
    } else if ((i & 0b111110111111) == 0b000100001001) {
      arm_lut[i] = arm_swp; // SWP
    } else if ((i & 0b111000001111) == 0b000000001011) {
      arm_lut[i] = arm_ldrh_strh; // LDRH, STRH
    } else if ((i & 0b111000011101) == 0b000000011101) {
      arm_lut[i] = arm_ldrsb_ldrsh; // LDRSB, LDRSH
    } else if ((i & 0b111110111111) == 0b000100000000) {
      arm_lut[i] = arm_mrs; // MRS
    } else if ((i & 0b111110111111) == 0b000100100000) {
      arm_lut[i] = arm_msr_reg; // MSR (reg)
    } else if ((i & 0b111110110000) == 0b001100100000) {
      arm_lut[i] = arm_msr_imm; // MSR (imm)
    } else if ((i & 0b111111111111) == 0b000100100001) {
      arm_lut[i] = arm_bx; // BX
    } else if ((i & 0b111000000001) == 0b000000000000) {
      arm_lut[i] = arm_data_proc_imm_shift; // Data Processing (imm shift)
    } else if ((i & 0b111000001001) == 0b000000000001) {
      arm_lut[i] = arm_data_proc_reg_shift; // Data Processing (reg shift)
    } else if ((i & 0b111110110000) == 0b001100000000) {
      arm_lut[i] = arm_undefined; // Undefined instructions in Data Processing
    } else if ((i & 0b111000000000) == 0b001000000000) {
      arm_lut[i] = arm_data_proc_imm; // Data Processing (imm value)
    } else if ((i & 0b111000000000) == 0b010000000000) {
      arm_lut[i] = arm_ldr_str_imm; // LDR, STR (immediate offset)
    } else if ((i & 0b111000000001) == 0b011000000000) {
      arm_lut[i] = arm_ldr_str_reg; // LDR, STR (register offset)
    } else if ((i & 0b111000000000) == 0b100000000000) {
      arm_lut[i] = arm_ldm_stm; // LDM, STM
    } else if ((i & 0b111000000000) == 0b101000000000) {
      arm_lut[i] = arm_branch; // B, BL
    } else if ((i & 0b111000000000) == 0b110000000000) {
      arm_lut[i] = arm_stc_ldc; // STC, LDC
    } else if ((i & 0b111100000001) == 0b111000000000) {
      arm_lut[i] = arm_cdp; // CDP
    } else if ((i & 0b111100000001) == 0b111000000001) {
      arm_lut[i] = arm_mcr_mrc; // MCR, MRC
    } else if ((i & 0b111100000000) == 0b111100000000) {
      arm_lut[i] = arm_swi; // SWI
    } else {
      arm_lut[i] = arm_undefined;
    }
  }
}
