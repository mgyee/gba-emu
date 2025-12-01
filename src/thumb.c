#include "cpu.h"
#include <stdio.h>

typedef int (*ThumbInstr)(CPU *cpu, Bus *bus, u16 instr);

static ThumbInstr thumb_lut[1024];

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

// Handler Stubs
static int thumb_unimpl(CPU *cpu, Bus *bus, u16 instr) {
  (void)cpu;
  (void)bus;
  printf("[THUMB] Unimplemented Instr: %04X\n", instr);
  return 0;
}

static int thumb_add_sub(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB ADD/SUB");
}
static int thumb_lsl_lsr_asr(CPU *cpu, Bus *bus, u16 instr) {
  printf("[THUMB] LSL/LSR/ASR: %04X\n", instr);
  Shift shift = GET_BITS(instr, 11, 2);
  u8 offset = GET_BITS(instr, 6, 5);
  u8 rs = GET_BITS(instr, 3, 3);
  u8 rd = GET_BITS(instr, 0, 3);

  ShiftRes res = barrel_shifter(cpu, shift, REG(rs), offset, true);
  REG(rd) = res.value;
  set_flags_nzc(cpu, res.value, res.carry);
}
static int thumb_mov_cmp_add_sub_imm(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB MOV/CMP/ADD/SUB IMM");
}
static int thumb_data_proc(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB DATA PROCESSING");
}
static int thumb_bx(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB BX");
}
static int thumb_high_reg_ops(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB HIGH REG OPS");
  printf("[THUMB] HIGH REG OPS: %04X\n", instr);
  u8 opcode = GET_BITS(instr, 8, 2);
  u8 rs = (instr & (1 << 6)) | GET_BITS(instr, 3, 3);
  u8 rd = (instr & (1 << 7)) | GET_BITS(instr, 0, 3);
}
static int thumb_ldr_pc_rel(CPU *cpu, Bus *bus, u16 instr) {
  printf("[THUMB] LDR PC REL: %04X\n", instr);
  u8 rd = GET_BITS(instr, 8, 3);
  u8 offset = GET_BITS(instr, 0, 8);

  u32 addr = ((PC - 2) & ~2) + (offset << 2);
  u32 val = bus_read32(bus, addr, ACCESS_NONSEQ);
  REG(rd) = val;

  return 1;
}
static int thumb_ldrh_strh_reg(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDRH/STRH REG");
}
static int thumb_ldrsh_ldrsb_reg(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDRSH/LDRSB REG");
}
static int thumb_ldr_str_reg(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDR/STR REG");
}
static int thumb_ldrb_strb_reg(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDRB/STRB REG");
}
static int thumb_ldr_str_imm(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDR/STR IMM");
}
static int thumb_ldrb_strb_imm(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDRB/STRB IMM");
}
static int thumb_ldrh_strh_imm(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDRH/STRH IMM");
}
static int thumb_ldr_str_sp_rel(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDR/STR SP REL");
}
static int thumb_add_sp_pc(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB ADD SP/PC");
}
static int thumb_add_sub_sp(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB ADD/SUB SP");
}
static int thumb_push_pop(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB PUSH/POP");
}
static int thumb_ldm_stm(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB LDM/STM");
}
static int thumb_swi(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB SWI");
}
static int thumb_undefined_bcc(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB UNDEFINED BCC");
}
static int thumb_bcc(CPU *cpu, Bus *bus, u16 instr) {
  printf("[THUMB] BCC: %04X\n", instr);
  u8 cond = GET_BITS(instr, 8, 4);
  u32 offset = GET_BITS(instr, 0, 8);
  if (offset & 0x80) {
    offset |= 0xFFFFFF00;
  }
  offset <<= 2;
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
    PC += offset;
    thumb_fetch(cpu, bus);
  }
  return 0;
}
static int thumb_branch(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB BRANCH");
}
static int thumb_bl_prefix(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB BL PREFIX");
}
static int thumb_bl_suffix(CPU *cpu, Bus *bus, u16 instr) {
  NOT_YET_IMPLEMENTED("THUMB BL SUFFIX");
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
      thumb_lut[i] = thumb_mov_cmp_add_sub_imm;
    } else if ((i & 0b1111110000) == 0b0100000000) {
      thumb_lut[i] = thumb_data_proc;
    } else if ((i & 0b1111111100) == 0b0100011100) { // 01000111.. BX
      thumb_lut[i] = thumb_bx;
    } else if ((i & 0b1111110000) == 0b0100010000) {
      thumb_lut[i] = thumb_high_reg_ops;
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
