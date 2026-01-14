#include "bus.h"
#include "cpu.h"
#include "gba.h"
#include <stdio.h>

typedef int (*ArmInstr)(Gba *gba, u32 instr);

static ArmInstr arm_lut[4096];

#ifdef DEBUG
static const char *alu_op_names[] = {"and", "eor", "sub", "rsb", "add", "adc",
                                     "sbc", "rsc", "tst", "teq", "cmp", "cmn",
                                     "orr", "mov", "bic", "mvn"};

static const char *shift_names[] = {"lsl", "lsr", "asr", "ror"};
#endif

static inline int arm_decode(u32 instr) {
  int high = (instr >> 16) & 0xFF0;
  int low = (instr >> 4) & 0x00F;
  return high | low;
}

u32 arm_fetch_next(Gba *gba) {
  u32 instr = gba->cpu.pipeline[0];
  gba->cpu.pipeline[0] = gba->cpu.pipeline[1];
  gba->cpu.pipeline[1] = bus_read32(gba, PC, gba->cpu.next_fetch_access);
  gba->cpu.next_fetch_access = ACCESS_SEQ;
  PC += 4;
  return instr;
}

void arm_fetch(Gba *gba) {
  gba->cpu.pipeline[0] = bus_read32(gba, PC, ACCESS_NONSEQ);
  gba->cpu.pipeline[1] = bus_read32(gba, PC + 4, ACCESS_SEQ);
  gba->cpu.next_fetch_access = ACCESS_SEQ;
  PC += 8;
}

int arm_step(Gba *gba) {
  u32 instr = arm_fetch_next(gba);

  if (!check_cond(&gba->cpu, instr)) {
#ifdef DEBUG
    printf("%08X: Cond not met\n", instr);
#endif
    return 0;
  }
  int index = arm_decode(instr);
  return arm_lut[index](gba, instr);
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
} ArmALUOpcode;

static int arm_undefined(Gba *gba, u32 instr) {
#ifdef DEBUG
  printf("%08X: undefined\n", instr);
#endif
  (void)gba;
  (void)instr;
  return 0;
}

// MUL, MLA
int arm_mul(Gba *gba, u32 instr) {
  bool acc = TEST_BIT(instr, 21);
  bool s = TEST_BIT(instr, 20);
  u8 rd = GET_BITS(instr, 16, 4);
  u8 rn = GET_BITS(instr, 12, 4);
  u8 rs = GET_BITS(instr, 8, 4);
  u8 rm = GET_BITS(instr, 0, 4);

#ifdef DEBUG
  if (acc) {
    printf("%08X: mla%s r%d, r%d, r%d, r%d\n", instr, s ? "s" : "", rd, rm, rs,
           rn);
  } else {
    printf("%08X: mul%s r%d, r%d, r%d\n", instr, s ? "s" : "", rd, rm, rs);
  }
#endif

  u8 m_cycles = 1;
  u32 tmp = rs ^ ((s32)rs >> 31);
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
    arm_fetch(gba);
  }

  if (s) {
    set_flags_nz(&gba->cpu, res);
  }

  return m_cycles;
}
// MULL, MLAL
int arm_mull(Gba *gba, u32 instr) {
  bool sign = TEST_BIT(instr, 22);
  bool acc = TEST_BIT(instr, 21);
  bool s = TEST_BIT(instr, 20);
  u8 rdhi = GET_BITS(instr, 16, 4);
  u8 rdlo = GET_BITS(instr, 12, 4);
  u8 rs = GET_BITS(instr, 8, 4);
  u8 rm = GET_BITS(instr, 0, 4);

#ifdef DEBUG
  printf("%08X: %s%s%s r%d, r%d, r%d, r%d\n", instr, sign ? "s" : "u",
         acc ? "mlal" : "mull", s ? "s" : "", rdlo, rdhi, rm, rs);
#endif

  u8 m_cycles = 2;
  u32 tmp = rs ^ ((s32)rs >> 31);
  m_cycles += (tmp > 0xff);
  m_cycles += (tmp > 0xffff);
  m_cycles += (tmp > 0xffffff);

  if (sign) {
    // SMULL, SMLAL
    s64 res = (s64)(s32)REG(rm) * (s64)(s32)REG(rs);
    if (acc) {
      res += ((s64)REG(rdhi) << 32) + (s64)REG(rdlo);
    }

    REG(rdlo) = res & 0xFFFFFFFF;
    REG(rdhi) = res >> 32;

    if (rdhi == 15 || rdlo == 15) {
      arm_fetch(gba);
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
      arm_fetch(gba);
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

int arm_swp(Gba *gba, u32 instr) {
  bool b = TEST_BIT(instr, 22);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 rm = GET_BITS(instr, 0, 4);

#ifdef DEBUG
  printf("%08X: swp%s r%d, r%d, [r%d]\n", instr, b ? "b" : "", rd, rm, rn);
#endif

  u32 val;

  if (b) {
    // byte
    val = bus_read8(gba, REG(rn), ACCESS_NONSEQ);
    bus_write8(gba, REG(rn), REG(rm) & 0xFF, ACCESS_NONSEQ);
  } else {
    // word
    u32 addr = REG(rn);
    val = bus_read32(gba, addr & ~3, ACCESS_NONSEQ);
    u32 rot = (addr & 3) * 8;
    if (rot) {
      val = (val >> rot) | (val << (32 - rot));
    }
    bus_write32(gba, addr & ~3, REG(rm), ACCESS_NONSEQ);
  }

  REG(rd) = val;

  if (rd == 15) {
    arm_fetch(gba);
  }

  return 1;
}

int arm_ldrh_strh(Gba *gba, u32 instr) {
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

#ifdef DEBUG
  const char *op = l ? "ldrh" : "strh";
  char addr_str[64];
  char off_sign = u ? '+' : '-';

  if (p) {
    // Pre-indexed
    if (i) {
      sprintf(addr_str, "[r%d, #%c%d]%s", rn, off_sign, offset, w ? "!" : "");
    } else {
      u8 rm = GET_BITS(instr, 0, 4);
      sprintf(addr_str, "[r%d, %cr%d]%s", rn, off_sign, rm, w ? "!" : "");
    }
  } else {
    // Post-indexed
    if (i) {
      sprintf(addr_str, "[r%d], #%c%d", rn, off_sign, offset);
    } else {
      u8 rm = GET_BITS(instr, 0, 4);
      sprintf(addr_str, "[r%d], %cr%d", rn, off_sign, rm);
    }
  }
  printf("%08X: %s r%d, %s\n", instr, op, rd, addr_str);
#endif

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
      val = bus_read16(gba, addr & ~1, ACCESS_NONSEQ);
      ShiftRes sh_res = barrel_shifter(&gba->cpu, SHIFT_ROR, val, 8, true);
      REG(rd) = sh_res.value;
    } else {
      REG(rd) = bus_read16(gba, addr, ACCESS_NONSEQ);
    }
    if (rd == 15) {
      arm_fetch(gba);
    }
    cycles = 1;
  } else {
    // STRH
    bus_write16(gba, addr & ~1, REG(rd), ACCESS_NONSEQ);
    gba->cpu.next_fetch_access = ACCESS_NONSEQ;
  }

  if (wb && (!l || rd != rn)) {
    REG(rn) += offset + ((rn == 15) << 3);
  }

  return cycles;
}

int arm_ldrsb_ldrsh(Gba *gba, u32 instr) {
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

#ifdef DEBUG
  const char *op = h ? "ldrsh" : "ldrsb";
  char addr_str[64];
  char off_sign = u ? '+' : '-';

  if (p) {
    // Pre-indexed
    if (i) {
      sprintf(addr_str, "[r%d, #%c%d]%s", rn, off_sign, offset, w ? "!" : "");
    } else {
      u8 rm = GET_BITS(instr, 0, 4);
      sprintf(addr_str, "[r%d, %cr%d]%s", rn, off_sign, rm, w ? "!" : "");
    }
  } else {
    // Post-indexed
    if (i) {
      sprintf(addr_str, "[r%d], #%c%d", rn, off_sign, offset);
    } else {
      u8 rm = GET_BITS(instr, 0, 4);
      sprintf(addr_str, "[r%d], %cr%d", rn, off_sign, rm);
    }
  }
  printf("%08X: %s r%d, %s\n", instr, op, rd, addr_str);
#endif

  if (!u) {
    offset = -offset;
  }

  bool wb = !p || w;

  u32 addr = rn_val + (p ? offset : 0);

  u32 val;

  if (h) {
    // Halfword
    if (addr & 1) {
      val = bus_read16(gba, addr & ~1, ACCESS_NONSEQ) >> 8;
      if (val & 0x80) {
        val |= 0xFFFFFF00;
      }
    } else {
      val = bus_read16(gba, addr, ACCESS_NONSEQ);
      if (val & 0x8000) {
        val |= 0xFFFF0000;
      }
    }
  } else {
    // Byte
    val = bus_read8(gba, addr, ACCESS_NONSEQ);
    if (val & 0x80) {
      val |= 0xFFFFFF00;
    }
  }

  REG(rd) = val;
  if (rd == 15) {
    arm_fetch(gba);
  }

  if (wb && (rd != rn)) {
    REG(rn) += offset + ((rn == 15) << 3);
  }

  return 1;
}

int arm_mrs(Gba *gba, u32 instr) {
  bool r = TEST_BIT(instr, 22);
  u8 rd = GET_BITS(instr, 12, 4);

#ifdef DEBUG
  printf("%08X: mrs r%d, %s\n", instr, rd, r ? "spsr" : "cpsr");
#endif

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

static void arm_do_msr(Gba *gba, u32 val, bool r, u8 field_mask) {
  u32 mask = 0;
  if (TEST_BIT(field_mask, 0))
    mask |= 0x000000FF; // c
  if (TEST_BIT(field_mask, 1))
    mask |= 0x0000FF00; // x
  if (TEST_BIT(field_mask, 2))
    mask |= 0x00FF0000; // s
  if (TEST_BIT(field_mask, 3))
    mask |= 0xFF000000; // f

  if (r) {
    // SPSR
    Mode mode = CPSR & 0x1F;
    if (mode != MODE_USR && mode != MODE_SYS) {
      SPSR = (SPSR & ~mask) | (val & mask);
    }
  } else {
    // CPSR
    Mode mode = CPSR & 0x1F;
    if (mode == MODE_USR) {
      mask &= 0xFF000000;
    }

    u32 new_cpsr = (CPSR & ~mask) | (val & mask);
    new_cpsr |= 0x10;
    cpu_set_mode(&gba->cpu, new_cpsr & 0x1F);
    CPSR = new_cpsr;
  }
}

int arm_msr_reg(Gba *gba, u32 instr) {
  bool r = TEST_BIT(instr, 22);
  u8 field_mask = GET_BITS(instr, 16, 4);
  u8 rm = GET_BITS(instr, 0, 4);
  u32 rm_val = REG(rm);
  if (rm == 15) {
    rm_val -= 4;
  }

#ifdef DEBUG
  char flags[5];
  int fi = 0;
  if (field_mask & 1)
    flags[fi++] = 'c';
  if (field_mask & 2)
    flags[fi++] = 'x';
  if (field_mask & 4)
    flags[fi++] = 's';
  if (field_mask & 8)
    flags[fi++] = 'f';
  flags[fi] = '\0';

  printf("%08X: msr %s_%s, r%d\n", instr, r ? "spsr" : "cpsr", flags, rm);
#endif

  arm_do_msr(gba, rm_val, r, field_mask);

  return 0;
}

int arm_msr_imm(Gba *gba, u32 instr) {
  bool r = TEST_BIT(instr, 22);
  u8 field_mask = GET_BITS(instr, 16, 4);
  u8 rotate = GET_BITS(instr, 8, 4);
  u8 imm = GET_BITS(instr, 0, 8);

  u32 val = imm;
  u32 rot_amt = rotate * 2;
  if (rot_amt) {
    val = (val >> rot_amt) | (val << (32 - rot_amt));
  }

#ifdef DEBUG
  char flags[5];
  int fi = 0;
  if (field_mask & 1)
    flags[fi++] = 'c';
  if (field_mask & 2)
    flags[fi++] = 'x';
  if (field_mask & 4)
    flags[fi++] = 's';
  if (field_mask & 8)
    flags[fi++] = 'f';
  flags[fi] = '\0';

  printf("%08X: msr %s_%s, #%d\n", instr, r ? "spsr" : "cpsr", flags, val);
#endif

  arm_do_msr(gba, val, r, field_mask);

  return 0;
}

int arm_bx(Gba *gba, u32 instr) {
  u8 rn = GET_BITS(instr, 0, 4);

#ifdef DEBUG
  printf("%08X: bx r%d\n", instr, rn);
#endif

  u32 target = REG(rn);
  if (rn == 15) {
    target -= 4;
  }
  bool thumb = target & 1;

  if (thumb) {
    CPSR |= CPSR_T;
    PC = target & ~1;
    thumb_fetch(gba);
  } else {
    CPSR &= ~CPSR_T;
    PC = target;
    arm_fetch(gba);
  }

  return 0;
}

static void arm_do_dproc(Gba *gba, ArmALUOpcode opcode, u32 op1, u32 op2, u8 rd,
                         bool s, bool carry) {

  Cpu *cpu = &gba->cpu;

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

  if (r15_dst && (opcode != ALU_TST && opcode != ALU_TEQ && opcode != ALU_CMP &&
                  opcode != ALU_CMN)) {

    if (CPSR & CPSR_T) {
      thumb_fetch(gba);
    } else {
      arm_fetch(gba);
    }
  }
}

int arm_data_proc_imm_shift(Gba *gba, u32 instr) {
  ArmALUOpcode op = (ArmALUOpcode)GET_BITS(instr, 21, 4);
  bool s = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 shift_amt = GET_BITS(instr, 7, 5);
  Shift shift_type = (Shift)GET_BITS(instr, 5, 2);
  u8 rm = GET_BITS(instr, 0, 4);

#ifdef DEBUG
  char op2_str[64];
  if (shift_amt == 0 && shift_type == SHIFT_LSL) {
    sprintf(op2_str, "r%d", rm);
  } else {
    sprintf(op2_str, "r%d, %s #%d", rm, shift_names[shift_type], shift_amt);
  }

  if (op == ALU_MOV || op == ALU_MVN) {
    printf("%08X: %s%s r%d, %s\n", instr, alu_op_names[op], s ? "s" : "", rd,
           op2_str);
  } else if (op == ALU_CMP || op == ALU_CMN || op == ALU_TST || op == ALU_TEQ) {
    printf("%08X: %s r%d, %s\n", instr, alu_op_names[op], rn, op2_str);
  } else {
    printf("%08X: %s%s r%d, r%d, %s\n", instr, alu_op_names[op], s ? "s" : "",
           rd, rn, op2_str);
  }
#endif

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  u32 rm_val = REG(rm);
  if (rm == 15) {
    rm_val -= 4;
  }

  ShiftRes sh_res =
      barrel_shifter(&gba->cpu, shift_type, rm_val, shift_amt, true);

  arm_do_dproc(gba, op, rn_val, sh_res.value, rd, s, sh_res.carry);
  return 0;
}

int arm_data_proc_reg_shift(Gba *gba, u32 instr) {
  ArmALUOpcode op = (ArmALUOpcode)GET_BITS(instr, 21, 4);
  bool s = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 rs = GET_BITS(instr, 8, 4);
  Shift shift_type = (Shift)GET_BITS(instr, 5, 2);
  u8 rm = GET_BITS(instr, 0, 4);

#ifdef DEBUG
  char op2_str[64];
  sprintf(op2_str, "r%d, %s r%d", rm, shift_names[shift_type], rs);

  if (op == ALU_MOV || op == ALU_MVN) {
    printf("%08X: %s%s r%d, %s\n", instr, alu_op_names[op], s ? "s" : "", rd,
           op2_str);
  } else if (op == ALU_CMP || op == ALU_CMN || op == ALU_TST || op == ALU_TEQ) {
    printf("%08X: %s r%d, %s\n", instr, alu_op_names[op], rn, op2_str);
  } else {
    printf("%08X: %s%s r%d, r%d, %s\n", instr, alu_op_names[op], s ? "s" : "",
           rd, rn, op2_str);
  }
#endif

  u32 rn_val = REG(rn);

  u32 rs_val = REG(rs);
  if (rs == 15) {
    rs_val -= 4;
  }

  u32 rm_val = REG(rm);

  ShiftRes sh_res =
      barrel_shifter(&gba->cpu, shift_type, rm_val, rs_val & 0xFF, false);

  arm_do_dproc(gba, op, rn_val, sh_res.value, rd, s, sh_res.carry);
  return 1;
}

int arm_data_proc_imm(Gba *gba, u32 instr) {
  ArmALUOpcode op = (ArmALUOpcode)GET_BITS(instr, 21, 4);
  bool s = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  u8 shift_amt = GET_BITS(instr, 8, 4) * 2;
  u8 imm = GET_BITS(instr, 0, 8);

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  ShiftRes sh_res = barrel_shifter(&gba->cpu, SHIFT_ROR, imm, shift_amt, false);

#ifdef DEBUG
  if (op == ALU_MOV || op == ALU_MVN) {
    printf("%08X: %s%s r%d, #%d\n", instr, alu_op_names[op], s ? "s" : "", rd,
           sh_res.value);
  } else if (op == ALU_CMP || op == ALU_CMN || op == ALU_TST || op == ALU_TEQ) {
    printf("%08X: %s r%d, #%d\n", instr, alu_op_names[op], rn, sh_res.value);
  } else {
    printf("%08X: %s%s r%d, r%d, #%d\n", instr, alu_op_names[op], s ? "s" : "",
           rd, rn, sh_res.value);
  }
#endif

  arm_do_dproc(gba, op, rn_val, sh_res.value, rd, s, sh_res.carry);
  return 0;
}

static int arm_ldr_str_common(Gba *gba, u32 instr, u32 offset) {
  bool p = TEST_BIT(instr, 24);
  bool b = TEST_BIT(instr, 22);
  bool w = TEST_BIT(instr, 21);
  bool l = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  bool wb = !p || w;

  u32 addr = rn_val + (p ? offset : 0);

  int cycles = 0;

  if (l) {
    // Load
    u32 val;
    if (b) {
      // LDRB
      val = bus_read8(gba, addr, ACCESS_NONSEQ);
    } else {
      // LDR
      val = bus_read32(gba, addr & ~3, ACCESS_NONSEQ);
      u32 rot = (addr & 3) * 8;
      if (rot) {
        val = (val >> rot) | (val << (32 - rot));
      }
    }
    REG(rd) = val;
    if (rd == 15) {
      arm_fetch(gba);
    }
    cycles = 1;
  } else {
    // Store
    u32 val = REG(rd);

    if (b) {
      // STRB
      bus_write8(gba, addr, val & 0xFF, ACCESS_NONSEQ);
    } else {
      // STR
      bus_write32(gba, addr & ~3, val, ACCESS_NONSEQ);
    }
    gba->cpu.next_fetch_access = ACCESS_NONSEQ;
  }

  if (wb && (!l || rd != rn)) {
    REG(rn) += offset + ((rn == 15) << 3);
  }

  return cycles;
}

int arm_ldr_str_imm(Gba *gba, u32 instr) {
  bool u = TEST_BIT(instr, 23);
  u32 offset = GET_BITS(instr, 0, 12);

#ifdef DEBUG
  bool p = TEST_BIT(instr, 24);
  bool b = TEST_BIT(instr, 22);
  bool w = TEST_BIT(instr, 21);
  bool l = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  const char *op = l ? (b ? "ldrb" : "ldr") : (b ? "strb" : "str");
  char off_sign = u ? '+' : '-';

  if (p) {
    printf("%08X: %s r%d, [r%d, #%c%d]%s\n", instr, op, rd, rn, off_sign,
           offset, w ? "!" : "");
  } else {
    printf("%08X: %s r%d, [r%d], #%c%d\n", instr, op, rd, rn, off_sign, offset);
  }
#endif

  if (!u) {
    offset = -offset;
  }

  return arm_ldr_str_common(gba, instr, offset);
}

int arm_ldr_str_reg(Gba *gba, u32 instr) {
  bool u = TEST_BIT(instr, 23);
  u8 rm = GET_BITS(instr, 0, 4);
  Shift shift_type = (Shift)GET_BITS(instr, 5, 2);
  u8 shift_amt = GET_BITS(instr, 7, 5);

#ifdef DEBUG
  bool p = TEST_BIT(instr, 24);
  bool b = TEST_BIT(instr, 22);
  bool w = TEST_BIT(instr, 21);
  bool l = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u8 rd = GET_BITS(instr, 12, 4);
  const char *op = l ? (b ? "ldrb" : "ldr") : (b ? "strb" : "str");
  char off_sign = u ? '+' : '-';
  char shift_str[32] = "";

  if (shift_amt > 0 || shift_type != SHIFT_LSL) {
    sprintf(shift_str, ", %s #%d", shift_names[shift_type], shift_amt);
  }

  if (p) {
    printf("%08X: %s r%d, [r%d, %cr%d%s]%s\n", instr, op, rd, rn, off_sign, rm,
           shift_str, w ? "!" : "");
  } else {
    printf("%08X: %s r%d, [r%d], %cr%d%s\n", instr, op, rd, rn, off_sign, rm,
           shift_str);
  }
#endif

  u32 rm_val = REG(rm);
  if (rm == 15) {
    rm_val -= 4;
  }

  ShiftRes sh_res =
      barrel_shifter(&gba->cpu, shift_type, rm_val, shift_amt, true);
  u32 offset = sh_res.value;

  if (!u) {
    offset = -offset;
  }

  return arm_ldr_str_common(gba, instr, offset);
}

int arm_ldm_stm(Gba *gba, u32 instr) {
  bool p = TEST_BIT(instr, 24);
  bool u = TEST_BIT(instr, 23);
  bool s = TEST_BIT(instr, 22);
  bool w = TEST_BIT(instr, 21);
  bool l = TEST_BIT(instr, 20);
  u8 rn = GET_BITS(instr, 16, 4);
  u16 list = GET_BITS(instr, 0, 16);

#ifdef DEBUG
  printf("%08X: %s%s r%d%s, {", instr, l ? "ldm" : "stm",
         p ? (u ? "ib" : "db") : (u ? "ia" : "da"), rn, w ? "!" : "");
  bool first_print = true;
  for (int i = 0; i < 16; i++) {
    if ((list >> i) & 1) {
      if (!first_print)
        printf(", ");
      printf("r%d", i);
      first_print = false;
    }
  }
  printf("}%s\n", s ? "^" : "");
#endif

  u32 rn_val = REG(rn);
  if (rn == 15) {
    rn_val -= 4;
  }

  bool transfer_pc = (list >> 15) & 1;

  int bytes = 0;
  int first = 0;

  if (list != 0) {
    for (int i = 15; i >= 0; i--) {
      if ((list >> i) & 1) {
        first = i;
        bytes += 4;
      }
    }
  } else {
    list = 1 << 15;
    first = 15;
    bytes = 64;
    transfer_pc = true;
  }

  Mode mode = CPSR & 0x1F;
  bool switch_to_user =
      s && (!l || !transfer_pc) && mode != MODE_USR && mode != MODE_SYS;

  if (switch_to_user) {
    cpu_set_mode(&gba->cpu, MODE_USR);
  }

  u32 addr = rn_val;
  u32 wb_addr = addr;
  if (u) {
    wb_addr += bytes;
  } else {
    addr -= bytes;
    wb_addr -= bytes;
  }
  if (u == p) {
    addr += 4;
  }

  Access access = ACCESS_NONSEQ;

  for (int i = first; i < 16; i++) {
    if ((list >> i) & 1) {
      if (l) {
        // Load
        u32 val = bus_read32(gba, addr, access);
        if (w && i == first) {
          REG(rn) = wb_addr;
        }
        REG(i) = val;
      } else {
        // Store
        bus_write32(gba, addr, REG(i), access);
        if (w && i == first) {
          REG(rn) = wb_addr;
        }
      }
      addr += 4;
      access = ACCESS_SEQ;
    }
  }

  if (l) {
    if (transfer_pc) {
      if (s) {
        u32 spsr = SPSR;
        cpu_set_mode(&gba->cpu, spsr & 0x1F);
        CPSR = spsr;
      }

      if (CPSR & CPSR_T) {
        thumb_fetch(gba);
      } else {
        arm_fetch(gba);
      }
    } else if (w && rn == 15) {
      arm_fetch(gba);
    }
  } else {
    if (w && rn == 15) {
      arm_fetch(gba);
    } else {
      gba->cpu.next_fetch_access = ACCESS_NONSEQ;
    }
  }

  if (switch_to_user) {
    cpu_set_mode(&gba->cpu, mode);
  }

  return l;
}

int arm_branch(Gba *gba, u32 instr) {
  bool link = TEST_BIT(instr, 24);
  s32 offset = GET_BITS(instr, 0, 24);

  if (offset & 0x800000) {
    offset |= 0xFF000000;
  }
  offset <<= 2;

#ifdef DEBUG
  printf("%08X: b%s %d\n", instr, link ? "l" : "", offset);
#endif

  if (link) {
    REG(14) = PC - 8;
  }

  PC += offset - 4;

  arm_fetch(gba);

  return 0;
}

int arm_stc_ldc(Gba *gba, u32 instr) {
  NOT_YET_IMPLEMENTED("STC/LDC");
  (void)gba;
  (void)instr;
}

int arm_cdp(Gba *gba, u32 instr) {
  NOT_YET_IMPLEMENTED("CDP");
  (void)gba;
  (void)instr;
}

int arm_mcr_mrc(Gba *gba, u32 instr) {
  NOT_YET_IMPLEMENTED("MCR/MRC");
  (void)gba;
  (void)instr;
}

int arm_swi(Gba *gba, u32 instr) {
#ifdef DEBUG
  u32 comment = GET_BITS(instr, 0, 24);
  printf("%08X: swi #%d\n", instr, comment);
#endif
  (void)instr;
  gba->cpu.spsr_svc = CPSR;
  cpu_set_mode(&gba->cpu, MODE_SVC);
  CPSR &= ~CPSR_T;
  CPSR |= CPSR_I;
  LR = PC - 8;
  PC = 0x08;
  arm_fetch(gba);
  return 0;
}

void arm_init_lut() {
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
