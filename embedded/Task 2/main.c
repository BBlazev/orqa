

#include <stdint.h>

typedef enum {
  BUTTON_RELEASED,
  BUTTON_PRESSED,
  BUTTON_TRANSITION
} ButtonState_t;

/**
* @brief Periodically debounces a digital input pin state using an
explicit state machine.
* @param raw_pin_state Immediate sample value of the target GPIO
pin (0 or 1).
* @return The evaluated, debounced state of the button interface.
*/

ButtonState_t debounce_button(uint8_t raw_pin_state) {
  // Implement internal state tracking and transitions here.

  static uint32_t counter = 0;
  static ButtonState_t state = BUTTON_RELEASED;

  if (state == BUTTON_RELEASED) {
    if (raw_pin_state == 0) {
      counter++;
      if (counter >= 3) {
        state = BUTTON_PRESSED;
        counter = 0;
      }
    } else
      counter = 0;

  }

  else if (state == BUTTON_PRESSED) {
    if (raw_pin_state == 1) {
      counter++;
      if (counter >= 3) {
        state = BUTTON_RELEASED;
        counter = 0;
      }
    } else
      counter = 0;
  }

  return state;
}
