
#include <stddef.h>
#include <stdint.h>

#define BASE 0x40021004U

// #define V_PTR_BASE (*(volatile uint32_t *)(BASE))

volatile uint32_t *reg = (volatile uint32_t *)BASE;

void set() { *reg |= (1U << 3); }

void clear() { *reg &= ~(1U << 10); }

void toggle() { *reg ^= (1U << 15); }

int main() { return 0; }
