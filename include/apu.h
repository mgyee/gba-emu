#pragma once
#include "common.h"

typedef struct {
  u16 soundbias;
} Apu;

void apu_init(Apu *apu);
