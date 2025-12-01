#include "cpu.h"
#include <stdio.h>

typedef int (*ThumbInstr)(CPU *cpu, Bus *bus, u16 instr);

static ThumbInstr thumb_lut[1024];

#ifdef DEBUG
static const char *thumb_shift_names[] = {"lsl", "lsr", "asr", "ror"};
#endif

static inline int thumb_decode(u16 instr) { return ((instr >> 6) & 0x3FF); }

u16 thumb_fetch_next(CPU *cpu, Bus *bus) {
  u16 instr = cpu->pipeline[0];
  cpu->pipeline[0] = cpu->pipeline[1];
  cpu->pipeline[1] = bus_read16(bus, PC, ACCESS_SEQ | ACCESS_CODE);
  PC += 2;
  return instr;
}

void thumb_fetch(CPU *cpu, Bus *bus) {
  cpu->pipeline[0] = bus_read16(bus, PC, ACCESS_NONSEQ | ACCESS_CODE);
  cpu->pipeline[1] = bus_read16(bus, PC + 2, ACCESS_SEQ | ACCESS_CODE);
  PC += 4;
}

typedef enum {
  THUMB_AND,
  THUMB_EOR,
  THUMB_LSL,
  THUMB_LSR,
  THUMB_ASR,
  THUMB_ADC,
  THUMB_SBC,
  THUMB_ROR,
  THUMB_TST,
  THUMB_NEG,
  THUMB_CMP,
  THUMB_CMN,
  THUMB_ORR,
  THUMB_MUL,
  THUMB_BIC,
  THUMB_MVN
} ThumbALUOpcode;

// Handler Stubs
static int thumb_unimpl(CPU *cpu, Bus *bus, u16 instr) {
  (void)cpu;
  (void)bus;
#ifdef DEBUG
  printf("%04X: unimplemented\n", instr);
#endif
  (void)instr;
  return 0;
}

static int thumb_add_sub(CPU *cpu, Bus *bus, u16 instr) {
  (void)bus;
  bool i = TEST_BIT(instr, 10);
  bool s = TEST_BIT(instr, 9);
  u8 rs = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

#ifdef DEBUG
  u8 imm_rn = GET_BITS(instr, 6, 3);
  if (i) {
    printf("%04X: %s r%d, r%d, #%d\n", instr, s ? "sub" : "add", rd, rs,
           imm_rn);
  } else {
    printf("%04X: %s r%d, r%d, r%d\n", instr, s ? "sub" : "add", rd, rs,
           imm_rn);
  }
#endif

  u32 op1 = REG(rs);
  u32 op2;
  if (i) {
    u8 imm = GET_BITS(instr, 6, 3);
    op2 = imm;
  } else {
    u8 rn = GET_BITS(instr, 6, 3);
    op2 = REG(rn);
  }

  u64 tmp;
  u32 res;
  bool carry = get_flag(cpu, CPSR_C);
  bool overflow = false;

  if (s) {
    tmp = (u64)op1 - op2;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
  } else {
    tmp = (u64)op1 + op2;
    res = (u32)tmp;
    carry = (tmp >> 32) & 1;
    overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
  }
  REG(rd) = res;
  set_flags(cpu, res, carry, overflow);
  return 0;
}
static int thumb_lsl_lsr_asr(CPU *cpu, Bus *bus, u16 instr) {
  (void)bus;
  Shift shift = GET_BITS(instr, 11, 2);
  u8 offset = GET_BITS(instr, 6, 5);
  u8 rs = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

#ifdef DEBUG
  printf("%04X: %s r%d, r%d, #%d\n", instr, thumb_shift_names[shift], rd, rs,
         offset);
#endif

  ShiftRes res = barrel_shifter(cpu, shift, REG(rs), offset, true);
  REG(rd) = res.value;
  set_flags_nzc(cpu, res.value, res.carry);
  return 0; // Added return 0 which was missing in original read (implicit int
            // return)
}
static int thumb_mov_cmp_add_sub(CPU *cpu, Bus *bus, u16 instr) {
  (void)bus;
  u8 opcode = GET_BITS(instr, 11, 2);
  u8 rd = GET_BITS(instr, 8, 3);
  u8 imm = GET_BITS(instr, 0, 8);

#ifdef DEBUG
  const char *mnem = "mov";
  if (opcode == 1)
    mnem = "cmp";
  else if (opcode == 2)
    mnem = "add";
  else if (opcode == 3)
    mnem = "sub";

  printf("%04X: %s r%d, #%d\n", instr, mnem, rd, imm);
#endif

  if (opcode == 0x0) { // MOV
    REG(rd) = imm;
    set_flags_nz(cpu, imm);
  } else {
    u32 op1 = REG(rd);
    u32 op2 = imm;
    u64 tmp;
    u32 res = 0;
    bool carry = get_flag(cpu, CPSR_C);
    bool overflow = false;
    switch (opcode) {
    case 0x1: // CMP
      tmp = (u64)op1 - op2;
      res = (u32)tmp;
      carry = !(tmp >> 32);
      overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
      break;
    case 0x2: // ADD
      tmp = (u64)op1 + op2;
      res = (u32)tmp;
      carry = (tmp >> 32) & 1;
      overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
      REG(rd) = res;
      break;
    case 0x3: // SUB
      tmp = (u64)op1 - op2;
      res = (u32)tmp;
      carry = !(tmp >> 32);
      overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
      REG(rd) = res;
      break;
    }
    set_flags(cpu, res, carry, overflow);
  }
  return 0;
}
static int thumb_data_proc(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: data_proc unimplemented\n", instr);
#endif
  ThumbALUOpcode opcode = (ThumbALUOpcode)GET_BITS(instr, 6, 4);
  u8 rs = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

  u32 op1 = REG(rd);
  u32 op2 = REG(rs);

  u32 res;
  ShiftRes shift_res;
  u64 tmp;
  bool carry = get_flag(cpu, CPSR_C);
  bool overflow = false;

  int cycles = 0;
  printf("Opcode: %d\n", opcode);

  switch (opcode) {
  case THUMB_AND:
    res = op1 & op2;
    REG(rd) = res;
    set_flags_nz(cpu, res);
    break;
  case THUMB_EOR:
    res = op1 ^ op2;
    REG(rd) = res;
    set_flags_nz(cpu, res);
    break;
  case THUMB_LSL:
    shift_res = barrel_shifter(cpu, SHIFT_LSL, op1, op2 & 0xFF, false);
    REG(rd) = shift_res.value;
    set_flags_nzc(cpu, shift_res.value, shift_res.carry);
    cycles += 1;
    break;
  case THUMB_LSR:
    shift_res = barrel_shifter(cpu, SHIFT_LSR, op1, op2 & 0xFF, false);
    REG(rd) = shift_res.value;
    set_flags_nzc(cpu, shift_res.value, shift_res.carry);
    cycles += 1;
    break;
  case THUMB_ASR:
    shift_res = barrel_shifter(cpu, SHIFT_ASR, op1, op2 & 0xFF, false);
    REG(rd) = shift_res.value;
    set_flags_nzc(cpu, shift_res.value, shift_res.carry);
    cycles += 1;
    break;
  case THUMB_ADC:
    tmp = (u64)op1 + op2 + carry;
    res = (u32)tmp;
    carry = (tmp >> 32) & 1;
    overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
    REG(rd) = res;
    set_flags(cpu, res, carry, overflow);
    break;
  case THUMB_SBC:
    tmp = (u64)op1 - op2 - !carry;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
    REG(rd) = res;
    set_flags(cpu, res, carry, overflow);
    break;
  case THUMB_ROR:
    shift_res = barrel_shifter(cpu, SHIFT_ROR, op1, op2 & 0xFF, false);
    REG(rd) = shift_res.value;
    set_flags_nzc(cpu, shift_res.value, shift_res.carry);
    cycles += 1;
    break;
  case THUMB_TST:
    res = op1 & op2;
    set_flags_nz(cpu, res);
    break;
  case THUMB_NEG:
    tmp = (u64)0 - op2;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((0 ^ op2) & (0 ^ res)) >> 31;
    REG(rd) = res;
    set_flags(cpu, res, carry, overflow);
    break;
  case THUMB_CMP:
    tmp = (u64)op1 - op2;
    res = (u32)tmp;
    carry = !(tmp >> 32);
    overflow = ((op1 ^ op2) & (op1 ^ res)) >> 31;
    set_flags(cpu, res, carry, overflow);
    break;
  case THUMB_CMN:
    tmp = (u64)op1 + op2;
    res = (u32)tmp;
    carry = (tmp >> 32) & 1;
    overflow = (~(op1 ^ op2) & (op2 ^ res)) >> 31;
    set_flags(cpu, res, carry, overflow);
    break;
  case THUMB_ORR:
    res = op1 | op2;
    REG(rd) = res;
    set_flags_nz(cpu, res);
    break;
  case THUMB_MUL:
    tmp = (u32)(rs ^ ((i32)rs >> 31));
    cycles += (tmp > 0xff);
    cycles += (tmp > 0xffff);
    cycles += (tmp > 0xffffff);

    res = REG(rd) * REG(rs);
    REG(rd) = res;
    set_flags_nz(cpu, res);
    break;
  case THUMB_BIC:
    res = op1 & ~op2;
    REG(rd) = res;
    set_flags_nz(cpu, res);
    break;
  case THUMB_MVN:
    res = ~op2;
    REG(rd) = res;
    set_flags_nz(cpu, res);
    break;
  }
  return cycles;
}
static int thumb_bx(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  u8 rn = GET_BITS(instr, 3, 4); // BX bits are 6-3 for Rm/Rn
  printf("%04X: bx r%d\n", instr, rn);
#endif
  NOT_YET_IMPLEMENTED("THUMB BX");
}
static int thumb_add_cmp_mov_hi(CPU *cpu, Bus *bus, u16 instr) {
  u8 opcode = GET_BITS(instr, 8, 2);
  u8 msbd = TEST_BIT(instr, 7) >> 4;
  u8 msbs = TEST_BIT(instr, 6) >> 3;
  u8 rs = msbs | GET_BITS(instr, 3, 3);
  u8 rd = msbd | GET_BITS(instr, 0, 3);

#ifdef DEBUG
  const char *mnem = "add";
  if (opcode == 1)
    mnem = "cmp";
  else if (opcode == 2)
    mnem = "mov";
  printf("%04X: %s r%d, r%d\n", instr, mnem, rd, rs);
#endif

  u32 op1 = REG(rd);
  u32 op2 = REG(rs);
  if (rd == 15) {
    op1 -= 2;
  }
  if (rs == 15) {
    op2 -= 2;
  }

  if (opcode == 0x1) { // CMP
    u64 tmp = (u64)op1 - op2;
    bool carry = !(tmp >> 32);
    bool overflow = ((op1 ^ op2) & (op1 ^ (u32)tmp)) >> 31;
    set_flags(cpu, (u32)tmp, carry, overflow);
  } else {
    if (opcode == 0x0) { // ADD
      REG(rd) = op1 + op2;
    }
    if (opcode == 0x2) { // MOV
      REG(rd) = op2;
    }
    if (rd == 15) {
      PC &= ~1;
      thumb_fetch(cpu, bus);
    }
  }
  return 0;
}
static int thumb_ldr_pc_rel(CPU *cpu, Bus *bus, u16 instr) {
  u8 rd = GET_BITS(instr, 8, 3);
  u8 offset = GET_BITS(instr, 0, 8);

#ifdef DEBUG
  printf("%04X: ldr r%d, [pc, #%d]\n", instr, rd, offset * 4);
#endif

  u32 addr = ((PC - 2) & ~2) + (offset << 2);
  u32 val = bus_read32(bus, addr, ACCESS_NONSEQ);
  REG(rd) = val;

  return 1;
}
static int thumb_ldrh_strh_reg(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldrh/strh reg unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDRH/STRH REG");
}
static int thumb_ldrsh_ldrsb_reg(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldrsh/ldrsb reg unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDRSH/LDRSB REG");
}
static int thumb_ldr_str_reg(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldr/str reg unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDR/STR REG");
}
static int thumb_ldrb_strb_reg(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldrb/strb reg unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDRB/STRB REG");
}
static int thumb_ldr_str_imm(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldr/str imm unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDR/STR IMM");
}
static int thumb_ldrb_strb_imm(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldrb/strb imm unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDRB/STRB IMM");
}
static int thumb_ldrh_strh_imm(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldrh/strh imm unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDRH/STRH IMM");
}
static int thumb_ldr_str_sp_rel(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldr/str sp rel unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDR/STR SP REL");
}
static int thumb_add_sp_pc(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: add sp/pc unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB ADD SP/PC");
}
static int thumb_add_sub_sp(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: add/sub sp unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB ADD/SUB SP");
}
static int thumb_push_pop(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: push/pop unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB PUSH/POP");
}
static int thumb_ldm_stm(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: ldm/stm unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB LDM/STM");
}
static int thumb_swi(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: swi unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB SWI");
}
static int thumb_undefined_bcc(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: undefined bcc unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB UNDEFINED BCC");
}
static int thumb_bcc(CPU *cpu, Bus *bus, u16 instr) {
  u8 cond = GET_BITS(instr, 8, 4);
  u32 offset = GET_BITS(instr, 0, 8);
  if (offset & 0x80) {
    offset |= 0xFFFFFF00;
  }
  offset <<= 1;

#ifdef DEBUG
  const char *cond_names[] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs",
                              "vc", "hi", "ls", "ge", "lt", "gt", "le"};
  if (cond < 14) {
    printf("%04X: b%s %d\n", instr, cond_names[cond], offset);
  } else {
    printf("%04X: bcc %d (cond=%d)\n", instr, offset, cond);
  }
#endif

  bool jump = false;
  switch (cond) {
  case 0x0: // EQ
    jump = get_flag(cpu, CPSR_Z);
    break;
  case 0x1: // NE
    jump = !get_flag(cpu, CPSR_Z);
    break;
  case 0x2: // CS/HS
    jump = get_flag(cpu, CPSR_C);
    break;
  case 0x3: // CC/LO
    jump = !get_flag(cpu, CPSR_C);
    break;
  case 0x4: // MI
    jump = get_flag(cpu, CPSR_N);
    break;
  case 0x5: // PL
    jump = !get_flag(cpu, CPSR_N);
    break;
  case 0x6: // VS
    jump = get_flag(cpu, CPSR_V);
    break;
  case 0x7: // VC
    jump = !get_flag(cpu, CPSR_V);
    break;
  case 0x8: // HI
    jump = get_flag(cpu, CPSR_C) && !get_flag(cpu, CPSR_Z);
    break;
  case 0x9: // LS
    jump = !get_flag(cpu, CPSR_C) || get_flag(cpu, CPSR_Z);
    break;
  case 0xA: // GE
    jump = (get_flag(cpu, CPSR_N) == get_flag(cpu, CPSR_V));
    break;
  case 0xB: // LT
    jump = (get_flag(cpu, CPSR_N) != get_flag(cpu, CPSR_V));
    break;
  case 0xC: // GT
    jump = !get_flag(cpu, CPSR_Z) &&
           (get_flag(cpu, CPSR_N) == get_flag(cpu, CPSR_V));
    break;
  case 0xD: // LE
    jump = get_flag(cpu, CPSR_Z) ||
           (get_flag(cpu, CPSR_N) != get_flag(cpu, CPSR_V));
    break;
  default:
    break;
  }
  if (jump) {
    PC += offset - 2;
    thumb_fetch(cpu, bus);
  }
  return 0;
}
static int thumb_branch(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: b unimplemented\n", instr);
#endif
  NOT_YET_IMPLEMENTED("THUMB BRANCH");
}
static int thumb_bl_prefix(CPU *cpu, Bus *bus, u16 instr) {
  u32 offset = GET_BITS(instr, 0, 11);
  if (offset & 0x400) {
    offset |= 0xFFFFF800;
  }
#ifdef DEBUG
  printf("%04X: bl prefix %d\n", instr, offset << 12);
#endif
  LR = PC - 2 + (offset << 12);

  return 0;
}
static int thumb_bl_suffix(CPU *cpu, Bus *bus, u16 instr) {
  u32 offset = GET_BITS(instr, 0, 11);
#ifdef DEBUG
  printf("%04X: bl suffix %d\n", instr, offset << 1);
#endif
  u32 lr = LR + (offset << 1);
  LR = (PC - 4) | 1;
  PC = lr & ~1;
  thumb_fetch(cpu, bus);
  return 0;
}

int thumb_step(CPU *cpu, Bus *bus) {
  u16 instr = thumb_fetch_next(cpu, bus);

  int index = thumb_decode(instr);
  // printf("[THUMB] Executing Instr: %04X\n", instr);
  return thumb_lut[index](cpu, bus, instr);
}

void thumb_lut_init() {
  for (int i = 0; i < 1024; i++) {
    if ((i & 0b1111100000) == 0b0001100000) {
      thumb_lut[i] = thumb_add_sub;
    } else if ((i & 0b1110000000) == 0b0000000000) {
      thumb_lut[i] = thumb_lsl_lsr_asr;
    } else if ((i & 0b1110000000) == 0b0010000000) {
      thumb_lut[i] = thumb_mov_cmp_add_sub;
    } else if ((i & 0b1111110000) == 0b0100000000) {
      thumb_lut[i] = thumb_data_proc;
    } else if ((i & 0b1111111100) == 0b0100011100) { // 01000111.. BX
      thumb_lut[i] = thumb_bx;
    } else if ((i & 0b1111110000) == 0b0100010000) {
      thumb_lut[i] = thumb_add_cmp_mov_hi;
    } else if ((i & 0b1111100000) == 0b0100100000) {
      thumb_lut[i] = thumb_ldr_pc_rel;
    } else if ((i & 0b1111011000) == 0b0101001000) {
      thumb_lut[i] = thumb_ldrh_strh_reg;
    } else if ((i & 0b1111011000) == 0b0101011000) {
      thumb_lut[i] = thumb_ldrsh_ldrsb_reg;
    } else if ((i & 0b1111011000) == 0b0101000000) {
      thumb_lut[i] = thumb_ldr_str_reg;
    } else if ((i & 0b1111011000) == 0b0101010000) {
      thumb_lut[i] = thumb_ldrb_strb_reg;
    } else if ((i & 0b1111000000) == 0b0110000000) {
      thumb_lut[i] = thumb_ldr_str_imm;
    } else if ((i & 0b1111000000) == 0b0111000000) {
      thumb_lut[i] = thumb_ldrb_strb_imm;
    } else if ((i & 0b1111000000) == 0b1000000000) {
      thumb_lut[i] = thumb_ldrh_strh_imm;
    } else if ((i & 0b1111000000) == 0b1001000000) {
      thumb_lut[i] = thumb_ldr_str_sp_rel;
    } else if ((i & 0b1111000000) == 0b1010000000) {
      thumb_lut[i] = thumb_add_sp_pc;
    } else if ((i & 0b1111111100) == 0b1011000000) {
      thumb_lut[i] = thumb_add_sub_sp;
    } else if ((i & 0b1111011000) == 0b1011010000) {
      thumb_lut[i] = thumb_push_pop;
    } else if ((i & 0b1111000000) == 0b1100000000) {
      thumb_lut[i] = thumb_ldm_stm;
    } else if ((i & 0b1111111100) == 0b1101111100) {
      thumb_lut[i] = thumb_swi;
    } else if ((i & 0b1111111100) == 0b1101111000) {
      thumb_lut[i] = thumb_undefined_bcc;
    } else if ((i & 0b1111000000) == 0b1101000000) {
      thumb_lut[i] = thumb_bcc;
    } else if ((i & 0b1111100000) == 0b1110000000) {
      thumb_lut[i] = thumb_branch;
    } else if ((i & 0b1111100000) == 0b1111000000) {
      thumb_lut[i] = thumb_bl_prefix;
    } else if ((i & 0b1111100000) == 0b1111100000) {
      thumb_lut[i] = thumb_bl_suffix;
    } else {
      thumb_lut[i] = thumb_unimpl;
    }
  }
}
