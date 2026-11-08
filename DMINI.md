# D MINI przez MIDI THRU

Zamiast stałego routingu USB CH4 → ścieżka 1 firmware ma teraz stronę
GLO → MIDI THRU: cztery przełączniki TRK1–TRK4 na pokrętłach 1–4,
domyślnie OFF. Ustawienia są zapisywane z konfiguracją urządzenia.

Dla D MINI włącz TRK1, pozostaw TRK2–TRK4 OFF. Kanał wyjściowy ustawia
GLO → MIDI → CH1. IN i OTHER działają jak dotychczas: np. IN = CH4
przypisuje klawiaturę na kanale 4 do wybranej ścieżki. Działa USB i TRS;
wyjście jest USB. Ścisz wewnętrzne brzmienie ścieżki 1, jeśli ma grać tylko D MINI.
Nie kieruj wyjścia MIDI z powrotem na wejście — THRU nie wykrywa pętli.

Przekazywane są tylko nuty i velocity, bez CC, sustainu i pitch bendu.
Note-off pamięta kanał wyjściowy także po zmianie wybranej ścieżki.
Zmiana konfiguracji, panic i przepełnienie wejścia kończą przekazane nuty.
Sekwencer i klawiatura grające tę samą nutę na tym samym kanale mogą
sobie przerwać dźwięk.

Testy hosta: tests/midi_thru_test.c, tests/midi_thru_settings_test.c,
tests/ui_pages_test.c i tests/hold_rec_test.c.
Pakiet zbudowany: build/felucca.fwsc. Kontrole pamięci firmware i integralności
pakietu przeszły. Brak testu na sprzęcie; budowanie opisuje BUILDING.md.
