#include "bus.h"
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
  return 0;
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
  (void)bus;
  ThumbALUOpcode opcode = (ThumbALUOpcode)GET_BITS(instr, 6, 4);
  u8 rs = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

#ifdef DEBUG
  static const char *thumb_alu_names[] = {
      "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
      "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn"};
  printf("%04X: %s r%d, r%d\n", instr, thumb_alu_names[opcode], rd, rs);
#endif

  u32 op1 = REG(rd);
  u32 op2 = REG(rs);

  u32 res;
  ShiftRes shift_res;
  u64 tmp;
  bool carry = get_flag(cpu, CPSR_C);
  bool overflow = false;

  int cycles = 0;

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
    tmp = (u32)(rs ^ ((s32)rs >> 31));
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
  u8 rn = GET_BITS(instr, 3, 4);
  printf("%04X: bx r%d\n", instr, rn);
#endif
  u8 msbs = TEST_BIT(instr, 6) >> 3;
  u8 rs = msbs | GET_BITS(instr, 3, 3);
  u32 target = REG(rs);
  if (rs == 15) {
    target -= 2;
  }
  bool thumb = target & 1;
  if (thumb) {
    CPSR |= CPSR_T;
    PC = target & ~1;
    thumb_fetch(cpu, bus);
  } else {
    CPSR &= ~CPSR_T;
    PC = target;
    arm_fetch(cpu, bus);
  }
  return 0;
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
static int thumb_ldr_str_reg(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  bool b = TEST_BIT(instr, 10);
  u8 ro = GET_BITS(instr, 6, 3);
  u8 rb = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

#ifdef DEBUG
  printf("%04X: %s%s r%d, [r%d, r%d]\n", instr, l ? "ldr" : "str", b ? "b" : "",
         rd, rb, ro);
#endif

  u32 addr = REG(rb) + REG(ro);

  if (l) {
    // Load
    if (b) {
      REG(rd) = bus_read8(bus, addr, ACCESS_NONSEQ);
    } else {
      u32 val = bus_read32(bus, addr, ACCESS_NONSEQ);
      u32 rot = (addr & 3) * 8;
      if (rot) {
        val = (val >> rot) | (val << (32 - rot));
      }
      REG(rd) = val;
    }
    return 1;
  } else {
    // Store
    if (b) {
      bus_write8(bus, addr, REG(rd) & 0xFF, ACCESS_NONSEQ);
    } else {
      bus_write32(bus, addr, REG(rd), ACCESS_NONSEQ);
    }
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}

static int thumb_ldrh_strh_reg(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  u8 ro = GET_BITS(instr, 6, 3);
  u8 rb = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

#ifdef DEBUG
  printf("%04X: %s r%d, [r%d, r%d]\n", instr, l ? "ldrh" : "strh", rd, rb, ro);
#endif

  u32 addr = REG(rb) + REG(ro);

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
    return 1;
  } else {
    // STRH
    bus_write16(bus, addr, REG(rd), ACCESS_NONSEQ);
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}
static int thumb_ldrsh_ldrsb_reg(CPU *cpu, Bus *bus, u16 instr) {
  bool h = TEST_BIT(instr, 11); // 0=LDRSB, 1=LDRSH
  u8 ro = GET_BITS(instr, 6, 3);
  u8 rb = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

#ifdef DEBUG
  printf("%04X: %s r%d, [r%d, r%d]\n", instr, h ? "ldrsh" : "ldrsb", rd, rb,
         ro);
#endif

  u32 addr = REG(rb) + REG(ro);
  u32 val;

  if (h) {
    // LDRSH
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
    // LDRSB
    val = bus_read8(bus, addr, ACCESS_NONSEQ);
    if (val & 0x80) {
      val |= 0xFFFFFF00;
    }
  }

  REG(rd) = val;
  return 1;
}

static int thumb_ldrb_strb_reg(CPU *cpu, Bus *bus, u16 instr) {
  return thumb_ldr_str_reg(cpu, bus, instr);
}

static int thumb_ldr_str_imm(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  u8 imm = GET_BITS(instr, 6, 5);
  u8 rb = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

  u32 offset = imm << 2;
  u32 addr = REG(rb) + offset;

#ifdef DEBUG
  printf("%04X: %s r%d, [r%d, #%d]\n", instr, l ? "ldr" : "str", rd, rb,
         offset);
#endif

  if (l) {
    u32 val = bus_read32(bus, addr, ACCESS_NONSEQ);
    u32 rot = (addr & 3) * 8;
    if (rot) {
      val = (val >> rot) | (val << (32 - rot));
    }
    REG(rd) = val;
    return 1;
  } else {
    bus_write32(bus, addr, REG(rd), ACCESS_NONSEQ);
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}
static int thumb_ldrb_strb_imm(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  u8 imm = GET_BITS(instr, 6, 5);
  u8 rb = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

  u32 offset = imm;
  u32 addr = REG(rb) + offset;

#ifdef DEBUG
  printf("%04X: %s r%d, [r%d, #%d]\n", instr, l ? "ldrb" : "strb", rd, rb,
         offset);
#endif

  if (l) {
    REG(rd) = bus_read8(bus, addr, ACCESS_NONSEQ);
    return 1;
  } else {
    bus_write8(bus, addr, REG(rd) & 0xFF, ACCESS_NONSEQ);
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}

static int thumb_ldrh_strh_imm(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  u8 imm = GET_BITS(instr, 6, 5);
  u8 rb = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

  u32 offset = imm << 1;
  u32 addr = REG(rb) + offset;

#ifdef DEBUG
  printf("%04X: %s r%d, [r%d, #%d]\n", instr, l ? "ldrh" : "strh", rd, rb,
         offset);
#endif

  if (l) {
    u16 val;
    if (addr & 1) {
      val = bus_read16(bus, addr, ACCESS_NONSEQ);
      ShiftRes sh_res = barrel_shifter(cpu, SHIFT_ROR, val, 8, true);
      REG(rd) = sh_res.value;
    } else {
      REG(rd) = bus_read16(bus, addr, ACCESS_NONSEQ);
    }
    return 1;
  } else {
    bus_write16(bus, addr, REG(rd), ACCESS_NONSEQ);
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}

static int thumb_ldr_str_sp_rel(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  u8 rd = GET_BITS(instr, 8, 3);
  u8 imm = GET_BITS(instr, 0, 8);

  u32 offset = imm << 2;
  u32 addr = SP + offset;

#ifdef DEBUG
  printf("%04X: %s r%d, [sp, #%d]\n", instr, l ? "ldr" : "str", rd, offset);
#endif

  if (l) {
    u32 val = bus_read32(bus, addr, ACCESS_NONSEQ);
    u32 rot = (addr & 3) * 8;
    if (rot) {
      val = (val >> rot) | (val << (32 - rot));
    }
    REG(rd) = val;
    return 1;
  } else {
    bus_write32(bus, addr, REG(rd), ACCESS_NONSEQ);
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}
static int thumb_add_sp_pc(CPU *cpu, Bus *bus, u16 instr) {
  (void)bus;
  bool sp = TEST_BIT(instr, 11); // 0=PC, 1=SP
  u8 rd = GET_BITS(instr, 8, 3);
  u8 imm = GET_BITS(instr, 0, 8);
  u32 val = imm << 2;

#ifdef DEBUG
  printf("%04X: add r%d, %s, #%d\n", instr, rd, sp ? "sp" : "pc", val);
#endif

  if (sp) {
    REG(rd) = SP + val;
  } else {
    REG(rd) = ((PC - 2) & ~2) + val;
  }
  return 0;
}

static int thumb_add_sub_sp(CPU *cpu, Bus *bus, u16 instr) {
  (void)bus;
  bool s = TEST_BIT(instr, 7);
  u8 imm = GET_BITS(instr, 0, 7);
  u32 val = imm << 2;

#ifdef DEBUG
  printf("%04X: %s sp, #%d\n", instr, s ? "sub" : "add", val);
#endif

  if (s) {
    SP -= val;
  } else {
    SP += val;
  }
  return 0;
}
static int thumb_push_pop(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  bool r = TEST_BIT(instr, 8);
  u8 list = GET_BITS(instr, 0, 8);

#ifdef DEBUG
  printf("%04X: %s {", instr, l ? "pop" : "push");
  bool first = true;
  for (int i = 0; i < 8; i++) {
    if (list & (1 << i)) {
      if (!first)
        printf(", ");
      printf("r%d", i);
      first = false;
    }
  }
  if (r) {
    if (!first)
      printf(", ");
    printf("%s", l ? "pc" : "lr");
  }
  printf("}\n");
#endif

  if (!list && !r) {
    if (l) {
      PC = bus_read32(bus, SP, ACCESS_NONSEQ);
      thumb_fetch(cpu, bus);
      SP += 64;
      return 1;
    } else {
      SP -= 64;
      bus_write32(bus, SP, PC, ACCESS_NONSEQ);
      bus->last_access = ACCESS_NONSEQ;
    }
    return 0;
  }

  if (l) {
    // POP
    u32 sp = SP;
    bus->last_access = ACCESS_NONSEQ;
    for (int i = 0; i < 8; i++) {
      if (list & (1 << i)) {
        REG(i) = bus_read32(bus, sp, ACCESS_SEQ);
        sp += 4;
      }
    }
    if (r) {
      PC = bus_read32(bus, sp, ACCESS_SEQ);
      PC &= ~1;
      sp += 4;
      thumb_fetch(cpu, bus);
    }
    SP = sp;
    return 1;
  } else {
    // PUSH
    u32 sp = SP;
    int count = 0;
    if (r)
      count++;
    for (int i = 0; i < 8; i++) {
      if (list & (1 << i))
        count++;
    }

    sp -= count * 4;
    u32 addr = sp;
    SP = sp;

    bus->last_access = ACCESS_NONSEQ;
    for (int i = 0; i < 8; i++) {
      if (list & (1 << i)) {
        bus_write32(bus, addr, REG(i), ACCESS_SEQ);
        addr += 4;
      }
    }
    if (r) {
      bus_write32(bus, addr, REG(14), ACCESS_SEQ);
    }
    bus->last_access = ACCESS_NONSEQ;
    return 0;
  }
}
static int thumb_ldm_stm(CPU *cpu, Bus *bus, u16 instr) {
  bool l = TEST_BIT(instr, 11);
  u8 rb = GET_BITS(instr, 8, 3);
  u8 list = GET_BITS(instr, 0, 8);

#ifdef DEBUG
  printf("%04X: %s r%d!, {", instr, l ? "ldmia" : "stmia", rb);
  bool first_print = true;
  for (int i = 0; i < 8; i++) {
    if (list & (1 << i)) {
      if (!first_print)
        printf(", ");
      printf("r%d", i);
      first_print = false;
    }
  }
  printf("}\n");
#endif

  if (!list) {
    if (l) {
      PC = bus_read32(bus, REG(rb), ACCESS_NONSEQ);
      thumb_fetch(cpu, bus);
    } else {
      bus_write32(bus, REG(rb), PC, ACCESS_NONSEQ);
    }
    REG(rb) += 64;
  }

  u32 addr = REG(rb);

  if (l) {
    bus->last_access = ACCESS_NONSEQ;
    for (int i = 0; i < 8; i++) {
      if (list & (1 << i)) {
        REG(i) = bus_read32(bus, addr, ACCESS_SEQ);
        addr += 4;
      }
    }

    if (~list & (1 << rb)) {
      REG(rb) = addr;
    }

    return 1;
  } else {
    int bytes = 0;
    u8 first = 0;

    for (int i = 7; i >= 0; i--) {
      if (list & (1 << i)) {
        first = i;
        bytes += 4;
      }
    }

    u32 wb_addr = REG(rb) + bytes;

    bus->last_access = ACCESS_NONSEQ;

    for (int i = 0; i < 8; i++) {
      if (list & (1 << i)) {
        bus_write32(bus, addr, REG(i), ACCESS_SEQ);
        if (i == first) {
          REG(rb) = wb_addr;
        }
        addr += 4;
      }
    }
    return 0;
  }
}
static int thumb_swi(CPU *cpu, Bus *bus, u16 instr) {
#ifdef DEBUG
  printf("%04X: swi\n", instr);
#endif
  cpu->spsr_svc = CPSR;
  cpu_set_mode(cpu, MODE_SVC);
  CPSR &= ~CPSR_T;
  CPSR |= CPSR_I;
  LR = PC - 4;
  PC = 0x08;
  arm_fetch(cpu, bus);
  return 0;
}

static int thumb_undefined_bcc(CPU *cpu, Bus *bus, u16 instr) {
  (void)cpu;
  (void)bus;
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
  s32 offset = GET_BITS(instr, 0, 11);
  if (offset & 0x400) {
    offset |= 0xFFFFF800;
  }
  offset <<= 1;

#ifdef DEBUG
  printf("%04X: b %d\n", instr, offset);
#endif

  PC += offset - 2;
  thumb_fetch(cpu, bus);
  return 0;
}
static int thumb_bl_prefix(CPU *cpu, Bus *bus, u16 instr) {
  (void)bus;
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
