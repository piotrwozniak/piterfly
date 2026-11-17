# piterfly 1.2.1 — przełączanie patternów

Wydanie lokalne z 17 listopada 2026, poprawki do 1.2.0.

## Poprawki

- Wybór patternu podczas odtwarzania loopa czeka do końca pełnego obiegu najdłuższej bieżącej ścieżki. Obliczenie uwzględnia LEN i DIV wszystkich ośmiu ścieżek, także obu perkusji, i zaokrągla czas w górę do pełnych taktów. Dla DIV 1/16: 16 kroków = jeden takt, 32 = dwa, 64 = cztery. Skrajny przypadek 64 kroków przy DIV 2BAR trwa pełne 128 taktów; szybki CHAIN również nie obcina go do 64 taktów. Prośba w połowie obiegu czeka na jego koniec.
- Przełączenie zachowuje aktualne brzmienia, silniki, presety, kity i sekwencje opuszczanego patternu. Zapis stanu następuje w momencie przejścia, więc obejmuje również edycję wykonaną podczas oczekiwania. Działa z panelu i przez edytor, podczas grania oraz po zatrzymaniu.
- Gdy cztery bufory oczekujących zmian są zajęte, przełączenie jest odrzucane z komunikatem **STOP TO SAVE MORE**. Bieżąca edycja pozostaje; zatrzymaj transport i poczekaj na zapis.
- Utworzenie nowego projektu i import roboczego loopa odłączają go od poprzedniego slotu, żeby kolejne przełączenie nie nadpisało poprzedniego patternu tym nowym materiałem.

Szybki CHAIN stosuje tę samą granicę przejścia. Zapisywana aranżacja SONG nadal używa jawnej wartości BARS każdej pozycji. Pattern o niecałkowitej liczbie taktów przełącza się na granicy taktu po jego pełnej długości, a nie w środku taktu.

## Zgodność i zapis

Zgodność z projektami **1.2.0** pozostaje: **FUNB**, 6160 bajtów, ustawienia **PER4**, edytor **protocol 19**, sześć syntezatorów i dwie perkusje. Nie ma migracji FUN1–FUNA ani zmiany układu flash. Edytor 1.2.0 jest zgodny.

Zmiany są najpierw przechowywane w RAM. **PENDING nie oznacza trwałego zapisu.** Zatrzymaj odtwarzanie, puść nuty i poczekaj na **PATTERNS SAVED**, zanim wyłączysz zasilanie. Roboczy loop bez przypisanego patternu należy najpierw zapisać do slotu.

## Pliki i odtworzenie wydania

Pakiet: `build/releases/piterfly-1.2.1/piterfly-1.2.1.fwsc`. Ten sam katalog zawiera binaria aplikacji i loadera, dokładne źródła, licencje, `BUILD_INFO.json` oraz `SHA256SUMS`. Produkt aktualizatora pozostaje **FM-1_1120101**.

```sh
sh build.sh --release 1.2.1
sh tests/run_tests.sh
python3 tools/prepare_release.py
```

Wymagane zależności i zmienne środowiskowe opisuje [BUILDING.md](../BUILDING.md). To wydanie przygotowano lokalnie; nie zmienia publicznego instalatora ani nie instaluje firmware na urządzeniu.

## Weryfikacja

Pełny zestaw testów hosta przeszedł, łącznie z porównaniami dźwięku native/WASM. Test offline PWA w Chromium pominięto: Playwright nie jest zainstalowany.

Test regresyjny `tests/pattern_switch_test.c` sprawdza granice 64-krokowego patternu, długość i DIV perkusji, zachowanie silnika/presetu/kitów i parametrów, przywracanie bazowej wartości po chwilowym parameter lock, przełączanie po zatrzymaniu, ponowny odczyt z flash oraz odmowę przełączenia przy pełnym buforze. Jest włączony do standardowego zestawu testów z ASan/UBSan.

Build docelowy przechodzi: obraz aplikacji 581 564 bajty; RAM 97 848/98 304 bajtów, low RAM 29 344/29 384 bajtów, pool 335 756/344 064 bajtów. Zużycie RAM pozostaje takie jak w 1.2.0. Kod obsługi projektów kompilowany jest z optymalizacją rozmiaru; współdzieli obliczanie długości i tabelę nazw patternów.

Wyniki pełnego zestawu i budowania są dostarczone w raporcie w katalogu wydania. Fizyczny FM-1 nie był dostępny do testu odsłuchowego ani sprawdzenia zachowania panelu; dotyczy to również wcześniejszego, nadal oczekującego testu całego układu 6+2.

[Polska instrukcja](PITERFLY.md) · [Obsługa ścieżek](TRACKS.md) · [Zmiany 1.2.0](RELEASE-1.2.0.md)
