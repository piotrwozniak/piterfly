# piterfly 1.2.7 — zmiany względem SLOOP

Piterfly to firmware dla **M-VAVE FM-1**, oparty na **SLOOP 2.8.5** i Felucca. Ma **sześć ścieżek syntezatora i dwie niezależne perkusje**. Wersja 1.2.7 dodaje [USB MIDI Host](#usb-midi-host-bez-komputera). Wersję 1.2.6 właściciel sprawdził na fizycznym FM-1. Nowy tryb USB Host zweryfikowano w testach programowych.

**Wersja 1.2.7 jest zgodna z projektami od 1.2.0.** Format **FUNB** ma 6160 bajtów. Edytor używa protokołu **19**, a ustawienia formatu **PER4**. Pakiet wydania to `piterfly-1.2.7.fwsc`. [Informacje o wydaniu 1.2.7](RELEASE-1.2.7.md).

**Gałki tracka i barwy:** na HOME, FX i pozostałych zwykłych stronach parametrów PRESETS wybiera track, a ALGORITHM zmienia jego barwę/preset (na perkusji — zestaw). W aktywnym edytorze NOTES oba pokrętła miały już funkcje: PRESETS nadal zmienia zoom, ALGORITHM wybiera track. SONG, menu ustawień i kombinacje z przytrzymanymi przyciskami zachowują swoje przypisania. Podczas swobodnego nagrania wybór tracka pozostaje zablokowany.

## Dwie perkusje: ścieżki 7 i 8

Wybierz ścieżkę **7** lub **8** pokrętłem PRESETS. Strony DRUMS, siatka kroków i gałki edytują wybraną perkusję. Każda ma własny wybór zestawu, 16 brzmień/linii, 64 kroki, TUNE i DECAY każdej linii, poziom, panoramę, wysyłki REV/DLY, filtr, slicer i insert. Każda ma sześć głosów; hi-hat jednej nie ucina drugiej.

Dla ścieżki 7 kanał MIDI ustawiasz przez **CH7** (domyślnie 7), dla ścieżki 8 przez **DRUM CH** (domyślnie 10). THRU działa osobno. LEVEL/REV/DLY ścieżki 7 są jej własnymi parametrami; ścieżka 8 zachowuje dotychczasowe globalne regulatory perkusji. Edytor z protokołem 19 obsługuje oba tory niezależnie.

Biblioteka sampli i definicje zestawów SYN1–SYN4 są wspólne. Wybrany zestaw, pattern i strojenie linii zapisują się osobno dla każdej perkusji. Sześć syntezatorów nadal dzieli pulę ośmiu głosów.

## Porównanie

| Obszar | Bazowy SLOOP 2.8.5 | piterfly |
| --- | --- | --- |
| Ścieżki syntezatora i perkusja | 3 + 1 | **6 + 2**, ze wspólną pulą 8 głosów syntezatora |
| Zapisane patterny całego urządzenia | 4, A–D | **24**, A1–F4, sześć banków po cztery |
| Kolejność SONG | Do 16 pozycji | **Do 128 pozycji**, po 1–64 takty |
| Osobne nazwane songi | Jeden bieżący projekt | **Do 8**, zależnie od wolnej wspólnej puli **352 KiB**; lista, zapis, wczytanie, usuwanie i eksport na komputer |
| Kroki jednego tracka | Do 64 | Do 64 — to istniejąca funkcja SLOOP, nie nowy limit |
| Własne sample | 4 banki po 80 KiB | **Brak banków użytkownika**; wszystkie cztery przeznaczono na patterny i songi |
| HOME | 16 pól: skrót dłuższego patternu lub bieżąca szesnastka | Wszystkie kroki w rzędach po 16, również podczas odtwarzania |
| Dolny pasek w GLO i innych menu parametrów | Jedna szesnastka; znacznik niewidoczny poza nią | Wszystkie kroki wybranego tracka i znacznik do kroku 64 |
| REC READY i podgląd tracków przy free take | Długi track ściskany do 16 pól | Osobne rzędy po 16 rzeczywistych kroków |
| Nawigacja po stronach parametrów | Licznik przy tytule na dole | Dodatkowo kompaktowe kropki na środku górnego paska |
| MIDI THRU | Bez osobnych przełączników tej wersji | **TRK1–TRK7 i DRUM ON/OFF**, na stronach MIDI THRU i THRU 4-7 |
| Oznaczenie urządzenia i interfejsu | SLOOP / Felucca | Logo arcade **piterfly**, nazwy USB `piterfly` i `piterfly Update` |

Pattern przechowuje cały loop: wszystkie osiem tracków, ich nuty, brzmienia i miks. **24 patterny nie oznaczają 24 kroków ani 24 tracków.**

## Ekrany wyświetlane na FM-1

Ilustracje w tej instrukcji odwzorowują **ekran urządzenia 240 × 240 pikseli**. Główna galeria pochodzi z wydania 1.1.0 (7+1); nowe podglądy SONG/SONGS opisano osobno. Starsze obrazy nie potwierdzają wyglądu drugiej perkusji w bieżącym układzie 6+2. Nie są zrzutami edytora WWW ani zdjęciami fizycznego wyświetlacza. Dane muzyczne są przykładowe. Kliknięcie obrazu otwiera jego pełny plik.

### HOME: nazwa, silnik i numer instrumentu (1.2.5)

Każdy track pokazuje nazwę brzmienia, silnik i numer **#XXX** (od #001). Dla tracków 7 i 8 jest to nazwa i numer zestawu perkusyjnego oraz silnik `drums`. Numer zaczyna się nad lewą krawędzią paska LEVEL. Długie nazwy są skracane tak, aby silnik pozostał widoczny. Napisy REC/SOLO/MUTE chwilowo zastępują numer. Nie ma dodatkowego licznika typu `023/153` ani osobnej sekcji nad trackami.

Zachowano oryginalne tracki wysokości 36 px od y=40, czcionki, siatkę kroków, paski LEVEL, dolne pokrętła i cienki wskaźnik pozycji instrumentu w nagłówku. Poprawki przełączania pełnych patternów i zapamiętywania LEVEL/STEPS/PAN oraz instrumentów pozostają bez zmian.

![HOME 1.2.5: nazwy, silniki i numery nad paskami LEVEL](../assets/piterfly/screenshots/home-instrument-1.2.5.png)

### HOME: grupy ścieżek 1–4 i 5–8

Na urządzeniu widać jednocześnie cztery ścieżki. Wybór ścieżki pokrętłem **PRESETS** przełącza grupę. Pierwszy ekran pokazuje **1–4**, drugi **5–8**, z perkusjami pod numerami 7 i 8. Ścieżki 1–6 są syntezatorami.

| HOME: ścieżki 1–4 | HOME: ścieżki 5–8 |
| --- | --- |
| <a href="../assets/piterfly/screenshots/home-instrument-1.2.5.png"><img src="../assets/piterfly/screenshots/home-instrument-1.2.5.png" width="360" alt="HOME: ścieżki 1–4"></a> | <a href="../assets/piterfly/screenshots/home-tracks-5-8-1.2.5.png"><img src="../assets/piterfly/screenshots/home-tracks-5-8-1.2.5.png" width="360" alt="HOME: ścieżki 5–8"></a> |

### SAVE: sześć banków i 24 sloty patternów

Przytrzymaj **SAVE**, aby otworzyć warstwę klawiszy z tytułem **SONG** i bankiem patternów. **KNOB 1** wybiera bank A–F. Krótkie naciśnięcie SAVE otwiera osobną stronę kolejności SONG z ponumerowanymi pozycjami. Każdy ekran pokazuje cztery sloty bieżącego banku; komplet sześciu ekranów poniżej pokazuje wszystkie **A1–F4**, czyli **24 miejsca** na patterny. FM-1 nie pokazuje ich jako jednej siatki 24 kafelków.

Górny rząd wybiera pattern. Zielony rząd służy do zapisu, czerwony do kasowania. W przykładzie A1 i A2 są zajęte, a pozostałe sloty puste; przygaszone nazwy nadal wskazują dostępne adresy.

| SAVE: bank A — A1–A4 | SAVE: bank B — B1–B4 | SAVE: bank C — C1–C4 |
| --- | --- | --- |
| <a href="../assets/piterfly/screenshots/save-bank-a.png"><img src="../assets/piterfly/screenshots/save-bank-a.png" width="240" alt="SAVE: bank A — A1–A4"></a> | <a href="../assets/piterfly/screenshots/save-bank-b.png"><img src="../assets/piterfly/screenshots/save-bank-b.png" width="240" alt="SAVE: bank B — B1–B4"></a> | <a href="../assets/piterfly/screenshots/save-bank-c.png"><img src="../assets/piterfly/screenshots/save-bank-c.png" width="240" alt="SAVE: bank C — C1–C4"></a> |

| SAVE: bank D — D1–D4 | SAVE: bank E — E1–E4 | SAVE: bank F — F1–F4 |
| --- | --- | --- |
| <a href="../assets/piterfly/screenshots/save-bank-d.png"><img src="../assets/piterfly/screenshots/save-bank-d.png" width="240" alt="SAVE: bank D — D1–D4"></a> | <a href="../assets/piterfly/screenshots/save-bank-e.png"><img src="../assets/piterfly/screenshots/save-bank-e.png" width="240" alt="SAVE: bank E — E1–E4"></a> | <a href="../assets/piterfly/screenshots/save-bank-f.png"><img src="../assets/piterfly/screenshots/save-bank-f.png" width="240" alt="SAVE: bank F — F1–F4"></a> |


### SONG: przejście ponad pozycję 64 i koniec przy 128

To rzeczywisty układ strony SONG na FM-1: pięć wierszy kolejności, numer pozycji, nazwa patternu i liczba taktów. W przykładzie kolejność ma 128 pozycji odwołujących się do A1 i A2. Po lewej widać pozycje **63–67**, po prawej **125–128**.

| SONG: pozycje 63–67 | SONG: ostatnia pozycja 128 |
| --- | --- |
| <a href="../assets/piterfly/screenshots/song-positions-63-67.png"><img src="../assets/piterfly/screenshots/song-positions-63-67.png" width="360" alt="SONG: pozycje 63–67"></a> | <a href="../assets/piterfly/screenshots/song-position-128.png"><img src="../assets/piterfly/screenshots/song-position-128.png" width="360" alt="SONG: ostatnia pozycja 128"></a> |

**Pozycje SONG** układają zapisane patterny w aranżację. Każda określa pattern i 1–64 takty odtwarzania. To osobna pojemność od **64 kroków nutowych ścieżki**.

### 64 kroki w menu i przy nagrywaniu

Dolny podgląd w GLO oraz widok REC READY zachowują rzeczywiste pozycje kroków w rzędach po 16.

| GLO: wybrana ścieżka | REC READY: pełne kroki |
| --- | --- |
| <a href="../assets/piterfly/screenshots/glo-64.png"><img src="../assets/piterfly/screenshots/glo-64.png" width="360" alt="GLO: wybrana ścieżka"></a> | <a href="../assets/piterfly/screenshots/rec-ready-64.png"><img src="../assets/piterfly/screenshots/rec-ready-64.png" width="360" alt="REC READY: pełne kroki"></a> |

[Pełna galeria ekranów urządzenia](SCREENSHOTS-1.1.0.md) to archiwum 1.1.0: pokazuje wszystkie pary parametrów GRANUL, DUCK oraz kanały i THRU dodanych ścieżek.

## Patterny i przycisk SAVE

Przytrzymaj **SAVE**, żeby otworzyć warstwę patternów. **Pierwszym z czterech pokręteł parametrów (KNOB 1)** wybierasz bank A–F. Nie chodzi o pokrętło SELECT.

Białe klawisze liczymy od lewej, od F3, jako 1–16.

| Białe klawisze przy trzymanym SAVE | Działanie |
| --- | --- |
| 1–4 | Wybór jednego z czterech patternów bieżącego banku |
| 5–8 | Zapis aktualnego loopa; na ekranie zielone dyskietki |
| 9–12 | Kasowanie odpowiedniego patternu; na ekranie czerwone kosze |
| 13 | Przełączenie LOOP / SONG, o ile nie trwa SONG REC |
| 14 | Uzbrojenie albo zakończenie SONG REC |
| 15 | Bez przypisanej akcji |
| 16, kafelek CHAIN | Otwarcie edytora kolejności SONG |

Podświetlają się wyłącznie **klawisze 1, 5, 9 i 13**, wskazując początki czterech grup. Kolory zielony i czerwony dotyczą ekranu. Obecny sterownik klawiszy obsługuje jasność, ale nie wybór kolorów RGB.

Podczas grania wybór czeka do końca pełnego obiegu najdłuższej ścieżki, z uwzględnieniem LEN i DIV, zaokrąglonego w górę do pełnych taktów. Przy DIV 1/16 pattern 32-krokowy trwa dwa takty, a 64-krokowy cztery; przełączenie nie ucina go po 16 krokach. Przy zatrzymanym odtwarzaniu wybór działa od razu.

Zmiany LEVEL, brzmień, instrumentów, kitów i sekwencji bieżącego patternu są zachowywane przy przełączeniu, także przez zwykłe LOAD na stronie SONG i w edytorze, również gdy zmienisz coś po wybraniu następnego patternu, ale przed jego startem. Dotyczy to patternu wybranego lub zapisanego przez warstwę SAVE i edytor. Roboczy loop bez przypisanego slotu trzeba najpierw zapisać. Zmiany oczekujące w RAM trafią do flash po zatrzymaniu i chwili ciszy. Jeśli cztery bufory są zajęte, **STOP TO SAVE MORE** blokuje przejście, chroniąc edycję. Zatrzymaj transport i poczekaj na **PATTERNS SAVED**, zanim wyłączysz zasilanie. W aktywnym trybie odtwarzania SONG ręczne przełączanie patternów jest blokowane.

Nadpisanie zajętego miejsca wymaga ponownego naciśnięcia tego samego klawisza zapisu w ciągu trzech sekund. Puszczenie SAVE, zmiana banku lub wybranie innej operacji kasuje potwierdzenie.

Kasowanie także wymaga drugiego naciśnięcia w ciągu trzech sekund, przy nadal trzymanym SAVE. Trzeba zatrzymać odtwarzanie. Aktualny roboczy loop pozostaje, ale **skasowanie zapisanego patternu nie ma Undo**. Jego pozycje są usuwane z kolejności SONG; jeżeli lista stanie się pusta, wraca domyślna kolejność.

Krótkie SAVE na HOME lub stronie DRUMS otwiera SONG. Na innych stronach może otwierać rodzinę menu SAVE; na samej stronie SONG zapisuje kolejność. Dostępne pozostaje także menu **SAVE → SECTION**, z wyborem A1–F4, kasowaniem, wczytaniem i zapisem.

## SONG, CHAIN i REC ARMED

SONG jest listą do **128 pozycji**. Każda wskazuje jeden z 24 patternów i długość 1–64 taktów. Ten sam pattern może pojawiać się wiele razy.

Przykład: A1 przez 4 takty → A2 przez 8 → A1 przez 4 → B3 przez 8.

Na stronie SONG cztery pokrętła oznaczają:

1. **ENTRY** — pozycja na liście.
2. **SECTION** — przypisany pattern A1–F4.
3. **BARS** — liczba taktów tej pozycji, od **1 do 64**, w metrum 4/4. Wartość jest zawsze widoczna po prawej, np. `16 bars`, także dla pustego slotu. **Żółty licznik w prawym dolnym rogu** pokazuje BARS aktualnie wybranej pozycji i reaguje na obrót K3. Pattern powtarza się przez wskazany czas; następnie SONG przechodzi do następnej pozycji. Aby odtwarzać go dłużej niż 64 takty, wybierz ten sam pattern w kolejnych pozycjach.
4. **LENGTH** — liczba pozycji całej listy, do 128.

Krótkie SAVE na tej stronie zapisuje kolejność. OCT− przełącza LOOP / SONG. Pozostałe funkcje strony, w tym zapis patternu przez REC i wczytanie po potwierdzeniu OCT+, pochodzą z SLOOP.

**REC ARMED pod SAVE** oznacza gotowość SONG REC do nagrywania kolejności przełączanych patternów. Nagrywanie rusza na początku taktu podczas odtwarzania wybranego zapisanego patternu. STOP lub ponowne SAVE + 14 kończy nagrywanie; wynikiem jest kolejność SONG wraz z czasami grania. To nie jest nagrywanie audio ani nowych nut.

**REC READY** to inny ekran: gotowość do nagrywania nut / free take, obsługiwana zwykłym przyciskiem REC.

Szybki CHAIN pozostaje odrębną funkcją: podczas grania loopa trzymaj SAVE, wybierz kolejno co najmniej dwa patterny i puść SAVE. Kolejność zostanie zapętlona. **Ten szybki łańcuch ma nadal limit 8 pozycji**; limit 128 dotyczy zapisywanej listy SONG.

## Jak czytać kroki

Na HOME każdy track ma własną siatkę. W dolnym pasku stron parametrów widzisz siatkę aktualnie wybranego tracka. REC READY stosuje taką samą zasadę rzędów:

- do 16 kroków: jeden rząd;
- 17–32: dwa rzędy;
- 33–48: trzy rzędy;
- 49–64: cztery rzędy.

Czytaj od lewej do prawej, następnie kolejny rząd. Pole w kolorze tracka oznacza nutę lub uderzenie; ciemne pole oznacza brak nowego zdarzenia. Nie jest to wykres głośności ani długości wybrzmiewania nuty.

**Biała obwódka** wskazuje krok aktualnie odtwarzany. W dolnym pasku **żółte narożniki** wskazują krok wybrany do edycji; na żółtym wypełnieniu są białe. Oba oznaczenia mogą występować równocześnie. Kolor nuty pozostaje wewnątrz pola.

Przykład dolnego paska: odtwarzany krok ma białą obwódkę, a wybrany do edycji — żółte narożniki.

![Dwa rozróżnione kursory na dolnym pasku](../assets/piterfly/screenshots/cursors.png)

Wybór ścieżki przełącza widoczną grupę HOME: 1–4 lub 5–8. Ścieżki 7 i 8 są perkusjami. PRESETS wybiera ścieżkę, ALGORITHM — jej brzmienie. Przy trzymanym GLO drugi czarny klawisz przełącza grupę obsługiwaną klawiszami mute/solo i czterema pokrętłami poziomów.

Widok **STEPS po przytrzymaniu SEQ** i siatka edycji **DRUMS** nadal pokazują po 16 kroków na stronę. Jest to celowe: każdemu krokowi odpowiada fizyczny biały klawisz. OCT− / OCT+ przełącza strony. W warstwie SEQ nie trzymaj wtedy klawisza kroku, bo kombinacje mają także funkcje edycji.

Na HOME wybrany track ma cienką, **stałą ramkę w kolorze swojego kafelka**, zgodnie z motywem. Podczas nagrywania ramka nagrywanego tracka staje się **czerwona** i pojawia się **REC**; po zakończeniu wybrany track wraca do swojego koloru. Ramka nie miga i nie używa etykiety ARM. Gotowość do rozpoczęcia nagrania przy zatrzymanym transporcie nadal obsługuje dotychczasowy ekran REC READY. Zasady wyboru tracka do nagrywania pozostają takie jak w SLOOP; ramka pokazuje aktualny stan silnika, nie dodaje osobnego wyboru celu.

Paski po prawej na HOME pokazują **ustawione poziomy głośności tracków**, a nie aktualne mierniki sygnału.

## Nawigacja i wygląd

W górnym wierszu stron parametrów, na wysokości BPM, znajdują się ciasno ustawione kropki. Liczba kropek odpowiada dostępnym stronom danej rodziny; aktywna używa akcentu motywu, pozostałe koloru palety. **Przy jednej stronie wskaźnika nie ma.** Tymczasowe komunikaty i MIDI LEARN mają pierwszeństwo w pasku statusu.

GLO zawiera także strony MIDI 4-7 oraz THRU 4-7 dla dodanych ścieżek. Kolejne naciśnięcie przycisku rodziny przełącza stronę; SELECT służy do przechodzenia pomiędzy stronami. Kropki nie są dodatkowymi przyciskami ani osobnym menu.

Między ikoną kasety a numerem tracka dodano odstęp. Kaseta oznacza wybrany track, nie nagrywanie. Logo urządzenia, instalatora i edytora jest w stylu arcade; nazwa `piterfly` jest pisana małą literą. Autorstwo SLOOP i Felucca pozostaje zachowane.

## Zapis: PENDING a SAVED

Pattern zapisany podczas grania najpierw trafia do roboczej pamięci RAM. **PENDING** oznacza, że czeka na trwały zapis. Na HOME stan ten pozostaje widoczny; jest przekazywany również do edytora WWW.

Po skasowaniu ścieżki przez przytrzymanie **REC** zatrzymaj odtwarzanie. **CLEAR SAVED** potwierdza zapis do flash.

Zatrzymaj odtwarzanie, puść nuty i daj urządzeniu chwilę bez obsługi. Zapis do flash czeka na ciszę i bezczynność; próby są ograniczone czasowo, więc nie musi nastąpić natychmiast po STOP. **PATTERNS SAVED** potwierdza zakończenie zapisu patternów. WWW również odróżnia oczekiwanie od zakończenia.

- Zapisane we flash patterny pozostają po odłączeniu zasilania.
- Zmiany nadal oznaczone PENDING mogą przepaść przy odłączeniu zasilania.
- Jednocześnie mogą oczekiwać cztery różne edytowalne patterny. Próba zapisu piątego zgłosi **STOP TO SAVE MORE**; zatrzymaj odtwarzanie i zaczekaj na zapis.
- Błąd zapisu ustawień lub kolejności SONG pozostawia zmiany do ponownej próby i zgłasza **SAVE ERROR: RETRYING**.

To nie jest gwarancja zachowania zmian przy dowolnej awarii sprzętu. Nie odłączaj zasilania w trakcie zapisu ani aktualizacji.

Zaznaczony wiersz **SONG** ma ramkę w kolorze patternu. Żółty licznik w prawym dolnym rogu pokazuje liczbę taktów.

![SONG: ramka w kolorze patternu i liczba taktów](../assets/piterfly/screenshots/song-order-1.2.6.png)

Podgląd z renderera firmware z przykładowymi danymi, nie zdjęcie urządzenia.

`EMPTY` na liście kolejności oznacza brak zapisanego patternu w slocie wybranym przez K2. Zmiana BARS nie tworzy patternu. **REC na stronie SONG** zapisuje bieżący loop do wybranego slotu (zastępowanie wymaga potwierdzenia), a status zmienia się na `READY`. `READY` oznacza dostępny pattern, nie potwierdzenie zakończenia zapisu flash — jeżeli widzisz PENDING, zaczekaj na trwały zapis. **SAVE na stronie SONG zapisuje kolejność**, a nie nuty. Pusta pozycja blokuje start SONG, dopóki nie zapiszesz w niej patternu albo nie wybierzesz innego, gotowego slotu.

## Kilka songów: zapis, lista, usuwanie i eksport

**Na urządzeniu możesz zapisać do 8 nazwanych songów, o ile wystarczy wolnej pamięci.** Nie jest to obietnica ośmiu utworów po 24 różne patterny: wszystkie songi i bieżąca praca dzielą **352 KiB**. Lista pokazuje wolne KiB na zapis.

1. Zatrzymaj odtwarzanie. Otwórz SONG, a następnie obróć **ALGORITHM**, aby wejść na listę SONGS.
2. **K1 / PRESETS** wybiera miejsce. **K2** wybiera pozycję w nazwie, **K3** zmienia znak; nazwa ma maksymalnie 15 znaków.
3. **SAVE** zapisuje pełną bieżącą kolejność, roboczy loop i niepuste patterny. Przy zastępowaniu istniejącego songa naciśnij SAVE drugi raz w ciągu 3 sekund.
4. **OCT+ dwa razy** wczytuje wybrany song z listy i zastępuje bieżącą pracę. **OCT− dwa razy** usuwa zapis songa. ALGORITHM albo HOME zamyka listę.
5. **REC dwa razy w ciągu 3 sekund** czyści bieżący loop, wszystkie 24 robocze sloty patternów i kolejność utworu. Komunikat **NEW SONG** potwierdza zapis tej zmiany. Nazwane utwory są nadal dostępne w bibliotece.

[Podgląd nowej listy SONGS](../assets/piterfly/screenshots/song-library-1.2.6.png) pochodzi z rzeczywistego renderera firmware z przykładowymi danymi; nie jest zdjęciem urządzenia.

Dotychczasowy SAVE na stronie kolejności SONG nadal zapisuje samą kolejność bieżącego projektu. Aby zachować osobny nazwany utwór, przejdź do listy SONGS.

Zapisywane są tylko patterny zawierające nuty lub uderzenia perkusji, również na wyciszonych trackach i poza bieżącą długością pętli. Pusty pattern z samymi ustawieniami brzmienia nie jest zachowywany; roboczy loop jest zachowywany zawsze. Niezmienione patterny mogą być współdzielone przez kilka songów. SYN1–SYN4 i bank patchy FM6 pozostają wspólną biblioteką urządzenia; nazwany song ani plik songa nie tworzy ich osobnej kopii. Zachowaj je przez backup, jeżeli ich używasz.

**NO SPACE: EXPORT/DELETE** oznacza, że zapis nie zmieścił się; poprzednia wersja zapisanego songa pozostaje dostępna. Nie jest to komunikat sukcesu. Usuwanie ma rezerwę miejsca, więc można je wykonać także po odmowie zapisu. Usunięcie zwalnia dane patternów i loopa nieużywane przez pozostałe songi ani bieżący projekt; 8 KiB wpisu kasowania pozostaje zajęte, aby starszy zapis nie wrócił po restarcie. Jeżeli usuniesz aktualnie wczytany song, jego robocza zawartość pozostaje. Wczytanie innego songa lub **NEW SONG** zwalnia jego niewspółdzielone dane.

**Na Macu:** w edytorze z tego brancha, w Song, wybierz zapis z listy i kliknij **Eksport na komputer**. Powstaje plik `.sloopsong`, bez wczytywania tego songa i bez automatycznego kasowania. Po sprawdzeniu pliku użyj **Usuń**, żeby zrobić miejsce. Plik można później wczytać dotychczasowym importem i ponownie zapisać w wybranym miejscu biblioteki. Dotychczasowy eksport bieżącego projektu nadal działa.

**Backup bieżącego projektu nie obejmuje całej listy zapisanych songów.** Przed aktualizacją lub zmianą firmware wyeksportuj każdy zapis z listy osobno. Szczegółowy rachunek pamięci: [SONGS.md](SONGS.md).

## Pamięć, migracja i zgodność

**Przed aktualizacją starego układu zrób backup.** Wszystkie USR1/2/3/4 służą do zapisu patternów i songów; sample użytkownika nie są zachowywane. Pozostają sample fabryczne i zestawy SYN1–SYN4.

Projekty 1.2.0–1.2.6 mają zgodny format **FUNB (6160 bajtów)**; aktualizacja między tymi wersjami zachowuje projekty. Stare patterny i projekty FUN1–FUNA nie są importowane; przy przejściu ze starszego formatu utwórz nowe patterny. Ustawienia urządzenia pozostają w PER4. Wspólna pula patternów i songów ma 352 KiB; nie ma banków sampli użytkownika.

W tym formacie usunięto pozycje USR z wyboru SAMPLE/GRAIN i drum kitów. SYN1–SYN4 mają nowe identyfikatory, dlatego również eksperymentalne projekty FUNA nie są importowane.

Używaj edytora dostarczonego z tym buildem. Obsługuje on 24 patterny i SONG do 128 pozycji, a także starsze urządzenia po odczytaniu ich możliwości. Pomoc oraz potwierdzenia importu podają pojemność podłączonego urządzenia.

**Import pliku utworu zastępuje wszystkie 24 miejsca patternów i roboczy loop**, także przez wyczyszczenie miejsc nieobecnych w pliku. Stary plik utworu z czterema patternami wypełni A1–A4, a pozostałe miejsca wyczyści. Nie jest to operacja „dodaj do wolnych miejsc”.

Przed pierwszym zapisem import sprawdza pola kolejności, powtórzone identyfikatory, długości i sumy kontrolne projektów oraz zgodność ustawień urządzenia. Błędny plik jest odrzucany przed modyfikacją patternów. Sam import wielu obiektów nie jest jednak jedną transakcją odporną na odłączenie kabla lub zasilania — zachowaj backup.

Edytor odmawia przywrócenia niepustych USR1/2/3/4 do tego builda oraz nowych ustawień PER4 na starszy firmware. Nie ma automatycznej migracji wstecz do układu czterech patternów; przy powrocie do SLOOP użyj backupu z wersji bazowej.

Szczegóły pamięci i protokołu: [PATTERNS-24.md](PATTERNS-24.md), [EDITOR_PROTOCOL.md](../web/EDITOR_PROTOCOL.md).

## USB MIDI Host bez komputera

**FM-1 → zasilany hub USB → syntezator USB MIDI.** Podłącz hub do jego zasilacza. Połącz port upstream huba z FM-1, a syntezator podłącz do portu danych huba. Zasilacz huba dostarcza 5 V urządzeniom takim jak Behringer JT-4000.

1. Przytrzymaj **HOME**, obróć **SELECT** do zakładki **USB** i przekręć **K3 (USB HOST)** w prawo.
2. Poczekaj na **MIDI READY**. Podczas wykrywania widać **LOOKING**. Przy **NO MIDI OUT** sprawdź zasilanie huba i przewody do transmisji danych. Użyj jednego huba. Jeśli hub USB-C nie wykrywa syntezatora, wypróbuj hub z portem danych USB-A.
3. Ustaw taki sam kanał MIDI na ścieżce FM-1 i w syntezatorze. Klawisze FM-1 wysyłają nuty. Aby wysłać także zapisane patterny, ustaw **GLO → MIDI 2 → OUT = SEQ**. **GLO → MIDI 3 → CLOCK = OUT** włącza wysyłanie zegara przy synchronizacji wewnętrznej.
4. Wyprowadź dźwięk z wyjścia audio syntezatora. Klawiaturę do FM-1 możesz podłączyć przez **TRS MIDI IN** i przekazywać nuty przełącznikiem THRU danej ścieżki.

Tryb używa **USB MIDI 1.0 OUT** i obsługuje jeden syntezator. Po włączeniu urządzenia USB HOST jest wyłączony. Przekręć **K3** w lewo, aby wrócić do połączenia z komputerem, edytorem, instalatorem lub USB Audio.

![Menu USB z K3 USB HOST i statusem MIDI READY. Render panelu z testu programowego.](../assets/piterfly/screenshots/menu-usb-host-1.2.7.png)

## MIDI THRU

Na stronie **GLO → MIDI THRU** ustawiasz TRK1–TRK3 i DRUM niezależnie na ON/OFF; dodane ścieżki TRK4–TRK7 mają stronę THRU 4-7. Funkcja przekazuje odebrane nuty według routingu wejścia i kanału wyjściowego tracka. Nie jest bezwarunkowym kopiowaniem wszystkich komunikatów MIDI.

Ustawienia THRU należą do konfiguracji urządzenia; wczytanie innego patternu ich nie zastępuje. Testy obejmują m.in. zmianę wybranego tracka przy trzymanej nucie, zwalnianie nut po zmianie kanału lub wyłączeniu THRU oraz niezależne wejścia USB/TRS.

**Przykład: Behringer D MINI na kanale MIDI 1.** Kontroler wysyła nuty na kanale 1 do wejścia USB lub TRS FM-1. Wyjście **USB MIDI OUT FM-1** kierujesz do Behringera przez komputer albo host/interfejs MIDI. Ustaw:

1. **GLO → MIDI → CH1 = 1** oraz odbiór w Behringerze na kanale **1**.
2. **GLO → MIDI 2 → IN = NOTES**, **OTHER = OFF** (dla opisywanego setupu).
3. **GLO → MIDI THRU → TRK1 = ON**; pozostałe przełączniki zostaw OFF, jeśli ich nie potrzebujesz.
4. Na HOME ustaw **LEVEL tracka 1 = 0**. Wewnętrzny dźwięk tego tracka cichnie, ale odebrane nuty nadal sterują Behringerem.

THRU przekazuje nuty na żywo niezależnie od OUT = KEYS / SEQ. Jeśli Behringer ma też odtwarzać nagrany pattern FM-1, ustaw **GLO → MIDI 2 → OUT = SEQ**. LEVEL zapisuje się w patternie, więc w patternach używanych do tego zestawu zachowaj zero na tracku 1. Obecna implementacja przekazuje Note On/Off i velocity; nie przekazuje CC, sustainu ani pitch bendu. Złącze TRS FM-1 jest wejściem, a przekazane MIDI wychodzi przez USB.

### Mac + MidiPipe + D MINI

[Oficjalna dokumentacja i pobieranie MidiPipe — SubtleSoft](http://www.subtlesoft.square7.net/MidiPipe.html). MyPipe to nazwa zestawu narzędzi routingu w MidiPipe.

[![Schemat połączenia klawiatury, FM-1, komputera i D MINI](../assets/piterfly/midi-rig.svg)](../assets/piterfly/midi-rig.svg)

Zielone strzałki pokazują drogę nut MIDI; żółte — osobne wyjścia audio. Wszystkie trzy urządzenia USB łączą się z komputerem osobnymi kablami do danych. Strzałka FM-1 ↔ komputer oznacza dwukierunkowe MIDI w jednym kablu USB.

Klawiatura USB, FM-1 i D MINI są osobno podłączone USB do Maca; dźwięk z FM-1 i D MINI trafia osobnymi połączeniami audio do miksera lub interfejsu. W jednym **MyPipe** narzędzia są ustawione od góry: **MIDI In: klawiatura → MIDI Out: piterfly (Pass Through OFF) → MIDI In: piterfly → MIDI Out: D MINI**. Firmware zgłasza port wejściowy i wyjściowy jako **piterfly**. Domyślne kanały to 1–6 dla syntezatorów, 7 dla perkusji na ścieżce 7 i 10 dla perkusji na ścieżce 8; główny pipe nie wymaga Channel Router ani Channel Filter. Zapisz go przez **File → Save** i zostaw MidiPipe uruchomiony.

D MINI odbiera tylko kanał 1. Track 1 ma LEVEL = 0, **CH1 = 1** i **OUT = SEQ**; tracki 2–6 oraz obie perkusje grają wewnętrznie. Instrument z klawiatury wybierasz jej kanałem nadawania, a nie zaznaczeniem tracka na ekranie FM-1.

W piterfly włącz TRK1 THRU dla nut granych na żywo. Ustawienie MidiPipe „Pass Through OFF” ma inną rolę: kontroluje przepływ w programie na Macu. Używaj jednej drogi nut do D MINI, aby ich nie dublować.

W serwisie, w sekcji MIDI THRU strony **What changes?**, dostępny jest angielski poradnik **FM-1 + Behringer D MINI setup guide** z osobnymi schematami USB i audio, kolejnością MyPipe i ekranami ustawień.

## Co sprawdzono

| Zakres | Wykonana walidacja |
| --- | --- |
| Pamięć patternów | Pełne 24 miejsca, rotacja zapisu, kasowanie, symulowane przerwanie programowania flash, ochrona zapisanych songów, zapis przy pełnej pamięci i usuwanie |
| Projekty 6+2 | Osobny kit, pattern i strojenie obu perkusji, zapis/odczyt wszystkich 24 miejsc; brak importu FUN1–FUNA |
| SONG | Przejście przez 128 pozycji, zakończenie i zapętlenie, timing audio |
| UI | Render do framebufferów, granice ekranu, ostatni krok 64, dwa kursory, 20 000 losowych klatek |
| Bezpieczeństwo zapisu | Symulowany błąd flash i ponowienie, wygaszenie potwierdzeń, PENDING → SAVED, także przez rzeczywisty kod protokołu MIDI |
| WWW | Testy protokołu 19 i audio WASM; niezależne kroki/strojenie obu perkusji, odrzucanie błędnych plików. Test wizualny/offline przeglądarki pominięto: brak Playwright |
| MIDI THRU | Routing, kanały, wyłączenie THRU, zwalnianie nut, USB/TRS i zachowanie ustawień |
| Build | Kompilacja, limity pamięci i weryfikacja paczki `.fwsc` |

**Aktualna wersja to 1.2.6.** Testy obejmują zapis po skasowaniu ścieżek, restart na symulowanej pamięci flash, tworzenie pustego utworu i ramkę w SONG. Test 6+2 na fizycznym FM-1 pozostaje do wykonania. [Informacje o wydaniu](RELEASE-1.2.6.md).

Najważniejsze testy 6+2: [two_drums_test.c](../tests/two_drums_test.c), [test_two_drums.mjs](../web/test_two_drums.mjs), [preview_test.mjs](../tests/preview_test.mjs). Pozostałe regresje obejmują też historyczne konfiguracje: [pattern_bank_test.c](../tests/pattern_bank_test.c), [pattern24_test.c](../tests/pattern24_test.c), [pattern24_migration_test.c](../tests/pattern24_migration_test.c), [arranger_test.c](../tests/arranger_test.c), [save_safety_test.c](../tests/save_safety_test.c), [ui_pages_test.c](../tests/ui_pages_test.c), [test_web.mjs](../web/test_web.mjs).

Ta dokumentacja ma pierwszeństwo nad limitami i opisami czterech sekcji w zachowanych instrukcjach bazowego SLOOP: [GUIDE.md](../GUIDE.md) i [SLOOP.md](../SLOOP.md). Licencja pozostaje GPL-3.0; informacje o autorach i pochodzeniu są w [README.md](../README.md).


## Granular na żywo

W piterfly **1.2.6** dostępny jest **FX → INSERT → GRANUL**, niezależnie na każdym tracku. Gałka 1 (INSRT) zawsze wybiera typ efektu, gałka 2 (PARAM) wybiera parę, a gałki 3/4 edytują: **SIZE/DENSITY**, **PITCH/MIX**, **POS/SPREAD** lub **CAPTURE/BUF**. BUF jest informacją o pojemności, bez edycji. SELECT normalnie przechodzi między stronami FX. Pojemność i stan LIVE/FREEZE są stale widoczne. BUF podaje rzeczywiście dostępny czas historii, zależny od liczby instancji, silników i pozostałych efektów.

Zmiana barwy, silnika lub presetu użytkownika zachowuje aktywny GRANUL, jego ustawienia i zamrożone audio. LIVE przechwytuje nową barwę. Edycja odrysowuje tylko zmienione pola, bez czyszczenia całego ekranu. Linie podziału, marginesy i pozycje wartości są takie same jak w CHOR; status bufora i LIVE/FREEZE nie przesuwa siatki.

Historia jest mono, 22,05 kHz, zapisana stratnie jako µ-law; suchy sygnał zachowuje dotychczasową jakość. Zapisują się ustawienia, ale nagrane audio znika po wyłączeniu zasilania lub załadowaniu projektu. Nowe projekty FUNB wymagają firmware 6+2 i edytora z protokołem 19; nie ma importu wcześniejszych projektów.

[Obsługa, ograniczenia i wyniki testów](LIVE-GRANULAR.md).


## DUCK i granular w 1.2.6

Wybierz ścieżkę syntezatora, otwórz **FX → FILTER** i pokrętłem **2** ustaw **DUCK ON/OFF**. Stopa ścisza tylko włączone cele; wspólny **MASTER → DUCK** określa głębokość. Stopy z obu perkusji wyzwalają DUCK. Celami są wyłącznie ścieżki syntezatora 1–6. Ustawienia celów zapisują się z patternem.

Praca ścieżek jest rozdzielana między oba rdzenie według zmierzonego kosztu. Przy **MIX = 0** granular po wygaszeniu efektu zatrzymuje przechwytywanie i przetwarzanie ziaren. Dotychczasowa historia pozostaje w pamięci; audio z okresu pauzy nie jest do niej dopisywane. Zwiększenie MIX przywraca efekt.

## Ekrany efektów i routingu

### GRANUL: parametry dźwięku i zamrożenie historii

Na INSERT wybierz GRANUL. Knob 2 zmienia parę parametrów; knobs 3 i 4 edytują pokazaną parę. Pierwszy ekran pokazuje SIZE/DENSITY, drugi CAPTURE/BUF z FREEZE. Pary PITCH/MIX i POS/SPREAD są dostępne pomiędzy nimi. BUF pokazuje aktualnie przydzielony czas historii.

| GRANUL: SIZE / DENSITY | GRANUL: FREEZE / BUF |
| --- | --- |
| <a href="../assets/piterfly/screenshots/piterfly-granular-sound.png"><img src="../assets/piterfly/screenshots/piterfly-granular-sound.png" width="360" alt="GRANUL: para SIZE i DENSITY"></a> | <a href="../assets/piterfly/screenshots/piterfly-granular-source.png"><img src="../assets/piterfly/screenshots/piterfly-granular-source.png" width="360" alt="GRANUL: CAPTURE ustawione na FREEZE i dostępny BUF"></a> |

### DUCK: wybór reakcji ścieżki na stopę

Na wybranej ścieżce syntezatora otwórz **FX → FILTER**. Drugi parametr **DUCK** przełącza ON/OFF dla tej ścieżki. Głębokość pozostaje we wspólnym MASTER → DUCK. Ten przełącznik dotyczy wewnętrznego audio FM-1.

<a href="../assets/piterfly/screenshots/piterfly-duck.png"><img src="../assets/piterfly/screenshots/piterfly-duck.png" width="240" alt="FILTER: przełącznik DUCK dla wybranej ścieżki syntezatora"></a>

### Kanały MIDI i THRU dla ścieżek 4–7

Dodatkowe ścieżki mają własne strony **MIDI 4-7** oraz **THRU 4-7**. Pierwsza ustawia ich kanały wyjściowe/wejściowe według routingu, druga niezależnie włącza przekazywanie nut. Oryginalna strona MIDI THRU zawiera TRK1–3 i DRUM.

| MIDI 4-7: kanały ścieżek | THRU 4-7: cztery osobne przełączniki |
| --- | --- |
| <a href="../assets/piterfly/screenshots/midi-tracks-4-7.png"><img src="../assets/piterfly/screenshots/midi-tracks-4-7.png" width="360" alt="MIDI 4-7: kanały dodanych ścieżek"></a> | <a href="../assets/piterfly/screenshots/midi-thru-tracks-4-7.png"><img src="../assets/piterfly/screenshots/midi-thru-tracks-4-7.png" width="360" alt="THRU 4-7: ON/OFF osobno dla ścieżek 4–7"></a> |
