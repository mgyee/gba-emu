#include "cpu.h"

int thumb_step(CPU *cpu, Bus *bus) { NOT_YET_IMPLEMENTED("Thumb step"); }

void thumb_fetch(CPU *cpu, Bus *bus) {
  cpu->pipeline[0] = bus_read16(bus, PC, ACCESS_NONSEQ | ACCESS_CODE);
  cpu->pipeline[1] = bus_read16(bus, PC + 2, ACCESS_SEQ | ACCESS_CODE);
  PC += 4;
}
