# Memory-Mapped Hardware Bit Manipulation

## Architectural Overview

Cilj je modificirati pojedinačne bitove peripheralnog kontrolnog registra
mapiranog na fiksnu adresu (`0x40021004`, veličina 32 bita), bez da se uništi
ostatak konfiguracije u registru.

- Registar se adresira preko **volatile**:
  ```c
  volatile uint32_t *reg = (volatile uint32_t *)BASE;
  ```
  `volatile` je na objektu na koji se pokazuje, pa kompajler ne smije
  optimizirati/cache-irati. Svaki read i write ide na memoriju,
  što je potrebno za hardverske registre. Adresa ima `U` sufiks (unsigned literal).
- Tri operacije, sve read-modify-write koje čuvaju ostale bitove:
  - **Set bit 3:** `*reg |= (1U << 3);`
  - **Clear bit 10:** `*reg &= ~(1U << 10);`
  - **Toggle bit 15:** `*reg ^= (1U << 15);`
- Maske koriste `1U` (unsigned) shift, čime se izbjegava undefined behaviour.

  Build:
   ```
   make 
   or
   gcc -std=c11 main.c -o bitops
   ```
