#include "bus.h"
#include "common.h"

#define PC (cpu->regs[15])

// Condition Flags
#define CPSR_N (BIT(31)) // Negative
#define CPSR_Z (BIT(30)) // Zero
#define CPSR_C (BIT(29)) // Carry
#define CPSR_V (BIT(28)) // Overflow

// Thumb Flag
#define CPSR_T (BIT(5))

typedef struct {
  u32 regs[16]; // R0-R15
  u32 cpsr;     // Current Program Status Register
} CPU;

void cpu_init(CPU *cpu);

int cpu_step(CPU *cpu, Bus *bus);
void arm_step(CPU *cpu, Bus *bus);
void thumb_step(CPU *cpu, Bus *bus);
