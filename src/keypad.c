#include "keypad.h"
#include <string.h>

void keypad_init(Keypad *keypad) {
  // keypad->keyinput.val = 0x03FF;
  // for (int i = 0; i < 10; i++) {
  //   keypad->keyinput.buttons[i] = 1;
  // }
  keypad->keyinput = 0x03FF;
  memset(&keypad->keycnt, 0, sizeof(keypad->keycnt));
}
