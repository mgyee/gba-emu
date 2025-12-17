#pragma once
#include "bus.h"
#include "common.h"

#define REG(x) (gba->cpu.regs[x])
#define PC (gba->cpu.r15)
#define LR (gba->cpu.r14)
#define SP (gba->cpu.r13)
#define CPSR (gba->cpu.cpsr)
#define SPSR (*(gba->cpu.spsr))

// Condition Flags
#define CPSR_N (BIT(31)) // Negative
#define CPSR_Z (BIT(30)) // Zero
#define CPSR_C (BIT(29)) // Carry
#define CPSR_V (BIT(28)) // Overflow

#define CPSR_I (BIT(7)) // IRQ Disable
#define CPSR_F (BIT(6)) // FIQ Disable

// Thumb Flag
#define CPSR_T (BIT(5))

typedef enum {
  MODE_USR = 0x10,
  MODE_FIQ = 0x11,
  MODE_IRQ = 0x12,
  MODE_SVC = 0x13,
  MODE_ABT = 0x17,
  MODE_UND = 0x1B,
  MODE_SYS = 0x1F
} Mode;

struct Cpu {
  union {
    struct {
      u32 r0;
      u32 r1;
      u32 r2;
      u32 r3;
      u32 r4;
      u32 r5;
      u32 r6;
      u32 r7;
      u32 r8;
      u32 r9;
      u32 r10;
      u32 r11;
      u32 r12;
      u32 r13;
      u32 r14;
      u32 r15;
    };
    u32 regs[16]; // R0-R15
  };

  u32 regs_usr[2];    // R13_usr - R14_usr
  u32 regs_usr_hi[5]; // R8_usr - R12_usr (Banked out during FIQ)
  u32 regs_fiq[7];    // R8_fiq - R14_fiq
  u32 regs_svc[2];    // R13_svc - R14_svc
  u32 regs_irq[2];    // R13_irq - R14_irq
  u32 regs_abt[2];    // R13_abt - R14_abt
  u32 regs_und[2];    // R13_und - R14_und

  u32 cpsr; // Current Program Status Register
  u32 *spsr;
  u32 spsr_fiq;
  u32 spsr_svc;
  u32 spsr_irq;
  u32 spsr_abt;
  u32 spsr_und;

  Access next_fetch_access;

  u32 pipeline[2]; // Instruction pipeline
};

void cpu_init(Cpu *cpu);
void cpu_set_mode(Cpu *cpu, u32 new_mode);

int cpu_step(Gba *gba);

void arm_init_lut();
int arm_step(Gba *gba);
void arm_fetch(Gba *gba);
u32 arm_fetch_next(Gba *gba);

void thumb_init_lut();
int thumb_step(Gba *gba);
void thumb_fetch(Gba *gba);
u16 thumb_fetch_next(Gba *gba);

bool check_cond(Cpu *cpu, u32 instr);

bool get_flag(Cpu *cpu, u32 flag);
void set_flags(Cpu *cpu, u32 res, bool carry, bool overflow);
void set_flags_nz(Cpu *cpu, u32 res);
void set_flags_nzc(Cpu *cpu, u32 res, bool carry);

typedef enum { SHIFT_LSL, SHIFT_LSR, SHIFT_ASR, SHIFT_ROR } Shift;
typedef struct {
  u32 value;
  bool carry;
} ShiftRes;

ShiftRes LSL(Cpu *cpu, u32 val, u32 amt);
ShiftRes LSR(Cpu *cpu, u32 val, u32 amt, bool imm);
ShiftRes ASR(Cpu *cpu, u32 val, u32 amt, bool imm);
ShiftRes ROR(Cpu *cpu, u32 val, u32 amt, bool imm);

ShiftRes barrel_shifter(Cpu *cpu, Shift shift, u32 val, u32 amt, bool imm);
