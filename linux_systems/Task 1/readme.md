# Thread-Safe Async Deque

## Architectural Overview

thread-safe double-ended queue (deque) na Linuxu.

- **Spremnik:** circular (ring) buffer (`int *buf`) s poljima `head`, `count` i
  `cap`. Sva memorija se alocira jednom, u `deque_init`, preko `malloc`. Nakon
  inicijalizacije push/pop operacije **ne rade nikakvu dinamičku alokaciju** –
  rade samo indeksnu aritmetiku po već rezerviranom bufferu
- **Sinkronizacija:** jedan `pthread_mutex_t` štiti stanje reda. Dvije
  condition varijable, `not_full` i `not_empty`, koriste se za blokirajuće
  čekanje (writeri čekaju na `not_full`, readeri na `not_empty`).
- **Način prisupanja:**
  - *Blokirajući:* `deque_push_back` / `deque_pop_front` čekaju na CV dok red ne
    postane spreman.
  - *Blokirajući s timeoutom:* `deque_push_back_timed` koristi
    `pthread_cond_timedwait` i vraća `-ETIMEDOUT` ako istekne rok.
  - *Non-blocking:* `deque_push_back_try` / `deque_pop_front_try` koriste
    `pthread_mutex_trylock` i vraćaju `-EAGAIN` ako je lock zauzet ili je red
    pun/prazan.
- **Operacije s oba kraja:** `push_back`/`pop_front` i `push_front`/`pop_back`
  omogućuju ponašanje deque-a; indeksi se računaju modulo `cap`.
- **Asinkrono multipleksiranje:** uz red je vezan `eventfd` (kreiran s
  `EFD_NONBLOCK | EFD_SEMAPHORE`). Svaki push poveća eventfd brojač (`write`),
  svaki pop ga smanji (`read`). Time red izlaže valjani file descriptor koji
  vanjski threadovi mogu pratiti preko `epoll`-a: `watcher` thread dodaje
  `ev_fd` u epoll i budi se kad ima `> 0` elemenata u redu.

## Assumptions & Trade-offs

- **Fiksni kapacitet:** kapacitet se zadaje pri inicijalizaciji i
  ne raste. Prednost je to što nema runtime alokacija.
- **Ring buffer:** kontinuirana memorija, bez alokacije po
  elementu, cache-friendly
- **`eventfd` s `EFD_SEMAPHORE`:** brojač eventfd-a prati broj elemenata, pa
  `epoll` signalizira spremnost čim ima podataka


## Verification Steps

1. Build:
   ```
   make 
   or
   gcc -std=c11 main.c -pthread -o deque_test
   ```
2. Pokreni `./deque_test`
3. Provjeri da ispisani povratni kodovi odgovaraju očekivanima označenima u
   ispisu (`want -EAGAIN`, `want -ETIMEDOUT`).