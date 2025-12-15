#include "keypad.h"
#include <string.h>

void keypad_init(Keypad *keypad) {
  memset(keypad, 0, sizeof(Keypad));
  keypad->keyinput = 0x03FF;
}
