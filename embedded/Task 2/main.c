#include <stdint.h>
#include <stdio.h>

typedef enum {
  BUTTON_RELEASED,
  BUTTON_PRESSED,
  BUTTON_TRANSITION
} ButtonState_t;

/**
 * @brief Periodically debounces a digital input pin state using an
 *        explicit state machine.
 * @param raw_pin_state Immediate sample value of the target GPIO pin (0 or 1).
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

  } else if (state == BUTTON_PRESSED) {
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

/* ----------------------- TEST DRIVER ----------------------- */

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

/*
 * Pusta zadani niz informacija kroz debounce_button (jedan uzorak = jedan poziv = jedan 5ms tick)
   i ispisuje pin i vraceno stanje za svaki tick.
 * Vraca zadnje stanje.
 */
static ButtonState_t run_sequence(const char *label, const uint8_t *samples, int n) {
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
  /*
   * debounce_button drzi stanje u static varijablama, pa se stanje
   * pamti kroz CIJELI program (kao u stvarnom ISR-u)
   *
   * stanja: 0 = Low = PRESSED, 1 = High = NOT pressed (released)
   * prijelaz se potvrdjuje tek nakon 3 uzastopna stanja (15 ms).
   */

  /* scenarij 1: bouncy pritisak.
   * Odskok (1) usred pritiska MORA resetirati brojac, pa se PRESSED potvrdjuje
   * tek nakon tri uzastopne 0, ne ranije.
   */

  uint8_t bouncy_press[] = {1, 0, 1, 0, 0, 0};
  ButtonState_t s1 = run_sequence("Scenarij 1: bouncy pritisak {1,0,1,0,0,0}",
                                  bouncy_press, 6);
  printf("--> Ocekivano na kraju: PRESSED. Dobiveno: %s\n", state_name(s1));

  /* Scenarij 2: drzanje pritisnutog.
   * Stalne 0 dok smo vec u PRESSED moraju drzati stanje mirnim (bez ponovnog
   * okidanja). */
  uint8_t hold[] = {0, 0, 0, 0};
  ButtonState_t s2 =
      run_sequence("Scenarij 2: drzanje pritisnutog {0,0,0,0}", hold, 4);
  printf("--> Ocekivano na kraju: PRESSED. Dobiveno: %s\n",
         state_name(s2));

  /* Scenarij 3: bouncy otpustanje.
   * Sada smo u PRESSED. Odskok (0) usred otpustanja resetira brojac, pa se
   * RELEASED potvrdjuje tek nakon tri uzastopna 1. */
  uint8_t bouncy_release[] = {1, 0, 1, 1, 1};
  ButtonState_t s3 = run_sequence(
      "Scenarij 3: bouncy otpustanje {1,0,1,1,1}", bouncy_release, 5);
  printf("--> Ocekivano na kraju: RELEASED. Dobiveno: %s\n", state_name(s3));

  /* Scenarij 4: kratki sum koji se NE smije potvrditi.
   * Pojedinacne 0 koje ne traju 3 uzastopna ticka ne smiju okinuti PRESSED. */
  uint8_t cut[] = {1, 0, 1, 0, 1, 0, 1};
  ButtonState_t s4 = run_sequence(
      "Scenarij 4: kratki sum {1,0,1,0,1,0,1}", cut, 7);
  printf("--> Ocekivano na kraju: RELEASED. Dobiveno: %s\n",
         state_name(s4));

  return 0;
}