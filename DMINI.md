# Piterfly 1.1.0 — D MINI przez MIDI THRU

MIDI THRU pozwala włączyć fizyczny syntezator do zestawu FM-1: nuty z klawiatury trafiają przez FM-1 do D MINI, a pozostałe ścieżki mogą grać wewnętrznie. Wydanie zostało sprawdzone na fizycznym FM-1 i zaakceptowane przez właściciela projektu.

## Połączenie

```text
Klawiatura USB ──→ komputer / host MIDI ──→ FM-1
                            ↑               │
                            └── MIDI OUT ───┘
                            │
                            └──→ D MINI

FM-1 AUDIO ──→ mikser / interfejs ←── D MINI AUDIO
```

Każde urządzenie USB łączy się z hostem. Sam kabel między dwoma urządzeniami USB nie zastępuje hosta. Audio obu instrumentów podłącz osobno do miksera; MIDI przekazuje nuty. Wejście TRS FM-1 także może odbierać nuty, ale przekazywane MIDI wychodzi przez USB.

## Przykład: ścieżka 1 steruje D MINI

1. Ustaw **CH1 = 1** w GLO → MIDI i kanał odbioru D MINI na **1**.
2. Ustaw **IN = NOTES**, **OTHER = OFF**. Klawiatura wysyła na kanale 1.
3. W GLO → MIDI THRU włącz **TRK1 ON**. Pozostałe THRU pozostaw OFF, jeśli ich nie używasz.
4. Wybierz ścieżkę 1 pokrętłem **PRESETS** i ustaw **LEVEL = 0**, aby grał sam D MINI. Zachowaj ten poziom w używanych patternach.
5. **OUT = SEQ** wysyła również zapisany pattern.
6. Host kieruje klawiaturę do FM-1 i wyjście MIDI FM-1 do D MINI.

W MidiPipe kolejność w jednym MyPipe to **MIDI In: klawiatura → MIDI Out: FM-1 (Pass Through OFF) → MIDI In: FM-1 → MIDI Out: D MINI**. Zapisz MyPipe i pozostaw aplikację uruchomioną. [Ilustrowana instrukcja](web/midipipe.html).

## Wszystkie osiem ścieżek

Domyślnie ścieżki syntezatora 1–7 używają kanałów MIDI 1–7, a perkusja (ścieżka 8) kanału 10. GLO → MIDI ustawia pierwotne ścieżki i perkusję; **MIDI 4-7** — dodane ścieżki. Pierwsza strona MIDI THRU zawiera TRK1–3 i DRUM, a **THRU 4-7** — TRK4–7. Ustawienia są zapisywane z konfiguracją urządzenia.

Każda ścieżka syntezatora może grać własnym silnikiem bez zewnętrznego sprzętu. Wyzerowanie LEVEL dotyczy jej wewnętrznego audio, nie wyłącza nut MIDI. MUTE i solo nadal sterują występem ścieżki.

THRU przekazuje **Note On/Off i velocity**, bez CC, sustainu i pitch bendu. Note-off pamięta kanał wyjściowy. Nie kieruj wyjścia z powrotem na wejście FM-1 — THRU nie wykrywa pętli. Sekwencer i klawiatura grające tę samą nutę na tym samym kanale mogą sobie przerwać dźwięk.

Granular i DUCK przetwarzają wewnętrzne audio FM-1. Nie przetwarzają osobnego wyjścia audio D MINI ani nie wysyłają automatyki jego głośności. Wspólne efekty lub ducking całego zestawu wymagają toru za mikserem.

[Wydanie 1.1.0](docs/RELEASE-1.1.0.md) · [Instrukcja](docs/PITERFLY.md) · [Budowanie](BUILDING.md)
