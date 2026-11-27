# Piterfly 1.2.9 — USB MIDI do sześciu syntezatorów

Wersja 1.2.9 dodaje **USB ROUTE** w **HOME → USB**. Przy wyłączonym USB HOST ustaw **K4 → TRACK 1-6** i włącz hosta pokrętłem **K3**. Użyj huba USB z własnym zasilaczem. Do gry na sześciu syntezatorach użyj sześciu portów danych dostępnych na jednym kontrolerze huba. Ścieżka syntezatora 1 wysyła MIDI na port 1 huba, ścieżka 2 na port 2 i tak dalej do portu 6. Kanał ustaw dla każdej ścieżki w GLO. **MIDI READY** sygnalizuje wykrycie co najmniej jednego odbiornika. Włącz **OUT = SEQ**, aby wysłać zapisane patterny, oraz **CLOCK = OUT**, aby wysyłać zegar do podłączonych syntezatorów. Ścieżki perkusyjne 7 i 8 grają wewnętrznie.

**ONE DEVICE** obsługuje pojedyncze urządzenie podłączone bezpośrednio lub przez hub. W tym trybie wszystkie ścieżki mogą wysyłać MIDI do wybranego odbiornika. K3 w lewo przywraca USB Audio, MIDI i połączenie z edytorem lub instalatorem. W obu trybach wyjście analogowe FM-1 i syntezatorów podłącz do odsłuchu osobno.

Bufory urządzenia USB i hosta współdzielą RAM. Zmiana roli zatrzymuje poprzednią transmisję przed inicjalizacją buforów. Zapis ikon współdzieli identyczne rzędy pikseli. Format projektów to **FUNB**, ustawień **PER4**, protokół edytora **19**. Wersja ma **24 patterny**, **128 pozycji SONG** i **352 KiB pamięci biblioteki songów**.

Testy programowe symulują sześć odbiorników, wysyłkę na ten sam kanał MIDI do różnych urządzeń, odpięcie i powrót odbiornika oraz przełączanie pomiędzy hostem i komputerem.

Kontroler możesz podłączyć przez **TRS MIDI IN** i włączyć THRU dla odpowiedniej ścieżki. W trybie **IN = NOTES** ustaw osobne kanały, jeśli kontroler ma wybierać odbiornik kanałem.

[Instrukcja i zrzuty ekranu](PITERFLY.md#usb-midi-host-bez-komputera) · [Instalator](https://piotrwozniak.github.io/piterfly/webapp/installer/)
