#include <stdint.h>
#include <stdio.h>

typedef enum {
  BUTTON_RELEASED,
  BUTTON_PRESSED,
  BUTTON_TRANSITION
} ButtonState_t;

/* Number of consecutive stable samples required to confirm a transition.
 * At a 5 ms tick this gives a 15 ms guard window */
#define DEBOUNCE_TICKS 3

/**
 * @brief Debounces a digital input by requiring N consecutive stable samples
 *        before committing a state change.
 *
 * @param raw_pin_state Immediate sample of the GPIO pin. Active-low wiring is
 *        assumed: 0 == button physically pressed, 1 == released.
 *
 * @return The current debounced state (BUTTON_RELEASED or BUTTON_PRESSED).
 *         BUTTON_TRANSITION is never returned -- B.B. decision
 */
ButtonState_t debounce_button(uint8_t raw_pin_state) {
  /* Persist across calls: each invocation is one timer tick, so the machine
   * must remember progress toward a transition between calls -- mirrors how a
   * real ISR retains state between fires. */
  static uint32_t counter = 0;
  static ButtonState_t state = BUTTON_RELEASED;

  if (state == BUTTON_RELEASED) {

    if (raw_pin_state == 0) {

      if (++counter >= DEBOUNCE_TICKS) {
        state = BUTTON_PRESSED;
        counter = 0;
      }
    } else {
      counter = 0;
    }

  } else if (state == BUTTON_PRESSED) {

    if (raw_pin_state == 1) {
      if (++counter >= DEBOUNCE_TICKS) {
        state = BUTTON_RELEASED;
        counter = 0;
      }
    } else {
      counter = 0;
    }
  }

  return state;
}

static const char *state_name(ButtonState_t s) {
  switch (s) {
  case BUTTON_RELEASED:
    return "RELEASED";
  case BUTTON_PRESSED:
    return "PRESSED";
  case BUTTON_TRANSITION:
    return "TRANSITION";
  default:
    return "?";
  }
}

static ButtonState_t run_sequence(const char *label, const uint8_t *samples,
                                  int n) {
  printf("\n=== %s ===\n", label);
  printf("tick | pin | state\n");
  printf("-----+-----+----------\n");

  ButtonState_t st = BUTTON_RELEASED;
  for (int i = 0; i < n; i++) {
    st = debounce_button(samples[i]);
    printf(" %3d |  %u  | %s\n", i, samples[i], state_name(st));
  }
  return st;
}

int main(void) {

  uint8_t bouncy_press[] = {1, 0, 1, 0, 0, 0};
  ButtonState_t s1 = run_sequence("Scenarij 1: bouncy pritisak {1,0,1,0,0,0}",
                                  bouncy_press, 6);
  printf("--> Ocekivano na kraju: PRESSED. Dobiveno: %s\n", state_name(s1));

  uint8_t hold[] = {0, 0, 0, 0};
  ButtonState_t s2 =
      run_sequence("Scenarij 2: drzanje pritisnutog {0,0,0,0}", hold, 4);
  printf("--> Ocekivano na kraju: PRESSED. Dobiveno: %s\n", state_name(s2));

  uint8_t bouncy_release[] = {1, 0, 1, 1, 1};
  ButtonState_t s3 = run_sequence("Scenarij 3: bouncy otpustanje {1,0,1,1,1}",
                                  bouncy_release, 5);
  printf("--> Ocekivano na kraju: RELEASED. Dobiveno: %s\n", state_name(s3));

  uint8_t cut[] = {1, 0, 1, 0, 1, 0, 1};
  ButtonState_t s4 =
      run_sequence("Scenarij 4: kratki sum {1,0,1,0,1,0,1}", cut, 7);
  printf("--> Ocekivano na kraju: RELEASED. Dobiveno: %s\n", state_name(s4));

  return 0;
}
