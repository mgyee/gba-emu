#pragma once
#include "common.h"

typedef enum {
  BUTTON_A,
  BUTTON_B,
  BUTTON_SELECT,
  BUTTON_START,
  BUTTON_RIGHT,
  BUTTON_LEFT,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_R,
  BUTTON_L
} Button;

struct Keypad {
  u16 keyinput;
  u16 keycnt;
};

void keypad_init(Keypad *keypad);
