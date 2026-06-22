# Button Debounce State Machine

## Architectural Overview

High-Reliability Button Debounce State Machine, izveden kao eksplicitni
state machine. Funkcija `debounce_button(raw_pin_state)` poziva se periodički
svakih 5 ms (iz ISR-a ili RTOS taska) pouzdano stanje tipkala.

- **Stanje tipke:** `raw_pin_state` 0 = Low = **PRESSED**, 1 = High = **NOT pressed**.
- **Interno stanje** drži se u `static` lokalnim varijablama (pamte se između
  poziva):
  - `state` – trenutno potvrđeno stanje: `BUTTON_RELEASED` ili
    `BUTTON_PRESSED`,
  - `counter` – broj uzastopnih pritisaka neke tipke.
- **Logika:** grananje ide **po trenutnom stanju**, ne po raw pinu:
  - Dok je stanje `RELEASED`: uzorak 0 (pritisak) povećava `counter`; uzorak 1
    resetira `counter` na 0. Nakon **3 uzastopna** uzorka 0
    (15 ms) stanje prelazi u `PRESSED`.
  - Dok je stanje `PRESSED`: uzorak 1 (otpuštanje) povećava `counter`; uzorak 0
    resetira `counter`. Nakon 3 uzastopna uzorka 1 stanje prelazi u `RELEASED`.
- `counter` se resetira čim je pritisak drugačiji od prošlog stanja pritiska.
  Prijelaz se potvrđuje tek nakon 3 uzastopna slažuća uzorka.

## Assumptions & Trade-offs

- **Prag od 3 uzorka (15 ms):** kompromis između responzivnosti i otpornosti. 
  Veći prag (npr. 6 uzastopna uzorka) = veća otpornost na odskoke, ali sporiji odziv; 3 uzorka dobro filtriraju bounce uz prihvatljivu latenciju.
- **`static` lokalno stanje:** jednostavno i bez dinamičke alokacije – pretpostavlja jednu  tipku i jednog pozivatelja. Za više tipki stanje bi se izdvojilo u strukturu po
  tipkiu koju pozivatelj prosljeđuje.
- **Početno stanje** je `BUTTON_RELEASED` (vrijednost 0 u enumu), što odgovara
  zadanom početnom stanju tipke.

## Verification Steps

Uz funkciju je priložen demo driver koji pušta nizove raw
uzoraka kroz `debounce_button` (svaki uzorak = jedan poziv = jedan 5 ms tick) i
ispisuje pin i vraćeno stanje za svaki tick.

> **Napomena:** `debounce_button` drži stanje u `static` varijablama, pa se stanje
> pamti kroz cijeli program (kao u stvarnom ISR-u). Zato je test jedan kontinuirani
> niz uzoraka koji simulira realnu vremensku liniju tipkala

Build i run:
```
make 
or
gcc -std=c11 -Wall -Wextra debounce_demo.c -o debounce_demo
./debounce_demo
```

1. **Bouncy** `{1,0,1,0,0,0}` – odskok (1) usred pritiska resetira
   brojač; PRESSED se potvrđuje tek nakon tri uzastopne 0.
2. **Držanje** `{0,0,0,0}` – stalne 0 drže PRESSED
3. **Bouncy 2** `{1,0,1,1,1}` – odskok (0) usred otpuštanja resetira
   brojač; RELEASED tek nakon tri uzastopne 1.
4. **Kratki prekidi** `{1,0,1,0,1,0,1}` – nijedan niz 0 ne traje 3 ticka, pa stanje
   ostaje RELEASED (šum odbijen).

Stvarni izlaz:
```
=== Scenarij 1: bouncy pritisak {1,0,1,0,0,0} ===
tick | pin | state
-----+-----+----------
   0 |  1  | RELEASED
   1 |  0  | RELEASED
   2 |  1  | RELEASED
   3 |  0  | RELEASED
   4 |  0  | RELEASED
   5 |  0  | PRESSED
--> Ocekivano na kraju: PRESSED. Dobiveno: PRESSED

=== Scenarij 2: drzanje pritisnutog {0,0,0,0} ===
tick | pin | state
-----+-----+----------
   0 |  0  | PRESSED
   1 |  0  | PRESSED
   2 |  0  | PRESSED
   3 |  0  | PRESSED
--> Ocekivano na kraju: PRESSED (mirno). Dobiveno: PRESSED

=== Scenarij 3: bouncy otpustanje {1,0,1,1,1} ===
tick | pin | state
-----+-----+----------
   0 |  1  | PRESSED
   1 |  0  | PRESSED
   2 |  1  | PRESSED
   3 |  1  | PRESSED
   4 |  1  | RELEASED
--> Ocekivano na kraju: RELEASED. Dobiveno: RELEASED

=== Scenarij 4: kratki sum {1,0,1,0,1,0,1} ===
tick | pin | state
-----+-----+----------
   0 |  1  | RELEASED
   1 |  0  | RELEASED
   2 |  1  | RELEASED
   3 |  0  | RELEASED
   4 |  1  | RELEASED
   5 |  0  | RELEASED
   6 |  1  | RELEASED
--> Ocekivano na kraju: RELEASED. Dobiveno: RELEASED
```
