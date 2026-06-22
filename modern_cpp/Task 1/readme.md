# FileWrapper – RAII wrapper za legacy C logging biblioteku

## Architectural Overview

`FileWrapper` enkapsulira jedan C-style `FILE*` handle i veže ga uz životni vijek objekta (RAII).

- **Konstruktor** otvara file preko `fopen` (`fopen(path, flag)`). Ako otvaranje
  ne uspije, baca `std::runtime_error`, pa objekt nikad ne postoji u nevaljanom
  stanju
- **Destruktor** automatski zatvara file preko `fclose`. Time se handle oslobađa
  čak i kod ranog `return`-a ili exception-a
- **Enkapsulacija:** raw `FILE*` je `private` i ne izlaže se nijednom javnom
  metodom. Pristup je moguć isključivo preko `read()` i `write()`.
  - `read()` čita file liniju po liniju (`fgets`) i vraća `std::vector<std::string>`.
  - `write()` zapisuje string preko `fputs` i baca exception ako zapis ne uspije.
- Kopiranje i premještanje su eksplicitno obrisani (`= delete`), čime se sprječava
  da dva objekta drže isti `FILE*` i dvaput ga zatvore (double `fclose`).

U `main.cpp` instanciraju se dva wrappera: prvi čita iz `input.txt`, a sadržaj se
sekvencijalno zapisuje u `output.txt` preko drugog wrappera.

## Assumptions & Trade-offs

- **Enkapsulacija:** namjerno ne postoji getter za sirovi
  `FILE*`. Pozivatelj koji bi htio `fread`/`fseek` način rada ne može
  do handlea, ali zauzvrat je nemoguće slučajno procuriti ili dvaput zatvoriti
  resurs.
- **Čitanje liniju po liniju u `std::vector<std::string>`:** jednostavno za
  korištenje i čitljivo, uz trošak da se cijeli file drži u memoriji. Za vrlo
  velike fileove streaming pristup bio bi memorijski jeftiniji, ovdje je
  prioritet bila jednostavnost.
- **Greške se javljaju exception-om**, ne return kodovima – usklađeno s ostatkom
  modernog C++ koda.

## Verification Steps

1. Build:
   ```
   make
   or
   g++ -std=c++20 main.cpp file_wrapper.cpp -o filewrapper
   ```
2. Pripremi `input.txt`.
3. Pokreni `./filewrapper`.
4. Usporedi ulaz i izlaz:
