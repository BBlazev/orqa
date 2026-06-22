#include <stddef.h>
#include <stdint.h>

/*
 * Memory-mapped GPIO register access.
 *
 * BASE is the absolute address of a hardware register.
 *
 * The pointer is declared `volatile` because the value behind this address
 * can change outside the C abstract machine (hardware sets/clears bits) and
 * our writes have side effects on hardware. Without `volatile` the compiler
 * is free to cache the value in a register, reorder, or elide the accesses
 * entirely -- all of which would silently break peripheral control.
 */
#define BASE 0x40021004U

#define PIN_SET    3U   /* bit driven high by set()    */
#define PIN_CLEAR  10U  /* bit driven low  by clear()  */
#define PIN_TOGGLE 15U  /* bit inverted    by toggle() */

volatile uint32_t *const reg = (volatile uint32_t *)BASE;

/**
 * @brief Atomically sets PIN_SET high, leaving others intact.
 * @note Read-modify-write: not interrupt-safe if an ISR touches the same reg.
 */
void set(void)    { *reg |=  (1U << PIN_SET); }

/**
 * @brief Clears PIN_CLEAR without disturbing other bits.
 */
void clear(void)  { *reg &= ~(1U << PIN_CLEAR); }

/**
 * @brief Inverts PIN_TOGGLE.
 */
void toggle(void) { *reg ^=  (1U << PIN_TOGGLE); }

int main(void) 
{ 
    return 0;
 }