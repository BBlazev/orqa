# ThreadPool – asinkroni worker pool

## Architectural Overview

`ThreadPool` upravlja fiksnim brojem worker threadova (default 4) koji u
pozadini izvršavaju zadatke.

- **Threadovi:** u konstruktoru se kreira `n` workera, svaki vrti `worker_task()`.
- **Queue:** zadaci se drže u `std::queue<std::function<void()>>`,
  zaštićenom s `std::mutex`. Buđenje i uspavljivanje workera ide preko
  `std::condition_variable`.
- **Worker logika:** worker uzme lock i čeka na CV preko
  `cv_.wait(lock,uvijet)`. Uvijet je „ima zadataka ILI je shutdown“. Time
  worker **spava** kad nema posla (nema busy waitinga). Kad zadatak stigne,
  worker se probudi, izvuče zadatak iz reda pod lockom, **otpusti lock**, pa
  izvrši zadatak izvan locka (da ne blokira ostale workere).
- **Submitanje s povratnom vrijednošću:** `enqueue` je generička, argument-forwarding
  metoda:
  ```cpp
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>>;
  ```
  Povratni tip se deducira preko `std::invoke_result_t`. Poziv se zapakira u
  `std::packaged_task<Ret()>` (preko `std::bind` s perfect-forwardanim argumentima),
  a taj `packaged_task` se drži u `std::shared_ptr`. Razlog za `shared_ptr`: 
  `packaged_task` je move-only i ne može se izravno spremiti u `std::function`
  (koji zahtijeva copyable callable), pa se u red gura lambda koja kopira
  `shared_ptr` i poziva `(*task)()`. Pozivatelj dobije `std::future` za rezultat.
- **Graceful shutdown:** u destruktoru se `shutdown` flag postavi **pod lockom**,
  zatim `notify_all()` probudi sve workere, pa se svi threadovi `join`-aju.
  Worker izlazi tek kad je `shutdown == true` **i** red prazan, pa se već
  zaqueueani zadaci stignu izvršiti prije gašenja.

## Assumptions & Trade-offs

- **Fiksan broj threadova** umjesto dinamičkog skaliranja: jednostavnije i
  predvidljivije.
- **`condition_variable` umjesto busy-waitinga:** worker u mirovanju ne troši
  CPU; trošak je zanemariva wake up latencija.
- **`enqueue` nakon shutdowna baca exception** (`std::runtime_error`) umjesto da
  tiho odbaci zadatak


## Verification Steps

1. Build:
   ```
   make 
   or
  
   g++ -std=c++20 main.cpp ThreadPool.cpp -pthread -o threadpool
   ```
2. Pokreni `./threadpool`. Test (`main.cpp`) pokriva više scenarija
3. Provjeri da svaki `future.get()` vrati očekivanu vrijednost i da svi void
   zadaci ispišu svoj redak točno jednom.
4. Program na kraju `main`-a uništi pool, svi preostali zadaci se izvrše, svi threadovi se     join-aju i program uredno izađe