# 🏎️ Nitro Drive

### 2D wyścigówka top-down napisana w C++ i SFML

---

## 👨‍🚀 Autorzy

Projekt zespołowy wykonany przez:

- **Maciej Drajewski**
- **Zofia Czelusta**

---

## 🎮 O grze

**Nitro Drive** to dynamiczna gra wyścigowa 2D w widoku z góry (top-down). Gracz wybiera tryb rozgrywki, ściga się z botami kierowanymi przez AI (lub samym sobą — na czas), zbiera po drodze monety i nitro, a zarobioną walutę wydaje w garażu na ulepszenia pojazdu i nowe malowania.

### 🛠️ Co już działa:

| Element | Opis |
|---|---|
| 🚗 **Fizyka pojazdu** | przyspieszenie, 6 biegów, obrotomierz (RPM), poślizg, nitro |
| 🤖 **Boty AI** | jazda po waypointach, 3 poziomy trudności, automatyczna zmiana biegów |
| 🪙 **Zbieranki** | monety i butelki nitro na trasie, odnawiające się po czasie |
| 🔧 **Garaż** | ulepszenia silnika, zawieszenia, nitro, skrzyni biegów, malowania |
| 💾 **Zapis postępu** | monety, poziomy ulepszeń i rekordy czasowe w pliku `save.dat` |
| 🖥️ **HUD** | prędkościomierz, poziom nitro, czas, pozycja w wyścigu |
| ✨ **Particle System** | dym z opon, spaliny, płomień nitro, iskry przy kolizjach |

---

## 🏁 Tryby gry

| Tryb | Opis |
|---|---|
| 🚀 **Drag Race** | Sprint na czas przeciwko 4 botom AI. Liczy się wyczucie zmiany biegów i moment użycia nitro. |
| 🔁 **Circuit** | 5 okrążeń przeciwko 4 botom AI — walka o jak najlepszą pozycję na mecie. |
| ⏱️ **Time Attack** | Samodzielna jazda na 3 okrążenia, bez przeciwników. Cel: pobicie własnego rekordu — na żywo widoczna różnica (delta) względem najlepszego okrążenia. |

---

## 🎮 Sterowanie

| Klawisz | Akcja |
|---|---|
| `W` / `↑` | Przyspieszanie |
| `S` / `↓` | Hamowanie / jazda do tyłu |
| `A` / `D`, `←` / `→` | Skręt w lewo / prawo |
| `Spacja` | Hamulec ręczny (poślizg / drift) |
| `E` | Aktywacja nitro |
| `Lewy Shift` | Zmiana biegu w górę |
| `Lewy Ctrl` | Zmiana biegu w dół |
| `F11` | Pełny ekran |
| `Esc` | Pauza / powrót do menu |

---

## ⚙️ Model fizyki pojazdu

Pojazd w Nitro Drive nie jest sterowany jako prosty wektor prędkości — symulowany jest uproszczony model jazdy obejmujący przyspieszenie i opory ruchu, biegi z obrotomierzem, poślizg podczas hamulca ręcznego oraz nitro.

### Jak to działa w praktyce?

**1. Przyspieszenie i opory ruchu**
Siła napędowa zależy od wciśnięcia gazu (`throttle`), parametru `acceleration` pojazdu oraz mnożnika nitro (`x1.35` przy aktywnym dopalaczu). Opór powietrza rośnie z kwadratem prędkości, a hamowanie odejmuje dodatkową siłę. Wynikowa prędkość jest ograniczana do maksimum wynikającego z aktualnego biegu i parametru `maxSpeed` silnika.

**2. Biegi i RPM**
Pojazd ma 6 biegów, każdy z przypisanym zakresem prędkości (`GEAR_MAX`). RPM jest przeliczane na podstawie pozycji aktualnej prędkości w zakresie danego biegu. Zmiana biegu w górę wymaga RPM ≥ 6500 i braku cooldownu po poprzedniej zmianie — zbyt częste przełączanie biegów jest krótko blokowane.

**3. Skręcanie**
Skuteczność skrętu zależy od prędkości — przy bardzo małej prędkości pojazd skręca słabo, a powyżej 80 km/h skręt jest dodatkowo tłumiony, żeby uniknąć nierealistycznych, "teleportujących się" zakrętów.

**4. Poślizg (drift)**
Wciśnięcie hamulca ręcznego przy prędkości > 20 wywołuje poślizg — nadwozie odchyla się od kierunku jazdy (`slideAngle`) proporcjonalnie do skrętu i odwrotnie proporcjonalnie do parametru `grip`. W poślizgu pojazd skręca mocniej, ale traci prędkość tym szybciej, im większy jest kąt poślizgu.

**5. Nitro**
Aktywacja (`E`) wymaga zapasu > 5, zużywa zbiornik w tempie ~25/s (pełny bak na ok. 3 sekundy) i daje mnożnik przyspieszenia `x1.35`. Zbiornik **nie regeneruje się samoczynnie** — uzupełnić go można jedynie zbierankami na trasie.

**6. Kolizje pojazd–pojazd**
Przy zderzeniu prędkość obu pojazdów spada proporcjonalnie do różnicy ich prędkości, aktywne nitro zostaje przerwane, a przy dużej stracie prędkości następuje automatyczna redukcja biegu.

---

## 🤖 Boty AI

Każdy bot jedzie po wyznaczonych punktach trasy (waypoints) i automatycznie przechodzi do następnego po zbliżeniu się na odległość poniżej 90 px. Poziom trudności wpływa na trzy parametry bota:

| Trudność | % maks. prędkości | Odchylenie od linii toru | Opóźnienie zmiany biegów | Nitro |
|---|---|---|---|---|
| 🟢 Easy | 68% | duże (±22 px) | wolne (0.6 s) | nie używa |
| 🟡 Medium | 82% | umiarkowane (±10 px) | szybkie (0.15 s) | przy zapasie > 55, na prostych |
| 🔴 Hard | 94% | minimalne (±4 px) | natychmiastowe (0.05 s) | przy zapasie > 35, na prostych |

Dodatkowo bot dynamicznie dostosowuje gaz i hamowanie do ostrości najbliższego zakrętu (od pełnego gazu na prostej do 25% gazu + hamowanie na ostrym zakręcie) oraz automatycznie zmienia biegi (`autoShift`) na podstawie aktualnych obrotów.

---

## 🔧 Garaż i ulepszenia

Za monety zebrane na trasie można w garażu rozwijać pojazd w pięciu kategoriach. Każde ulepszenie ma realny wpływ na statystyki samochodu (oprócz skrzyni biegów, na razie wyłącznie kosmetyczna/progresowa).

### 🔥 Silnik — wpływa na `maxSpeed` i `acceleration`

| Poziom | Nazwa | Koszt | Prędkość max. | Przyspieszenie |
|---|---|---|---|---|
| 0 | Stock 1.6T | — (domyślny) | 180 km/h | 90 |
| 1 | Engine 2.0T | $800 | 202 km/h | 104 |
| 2 | Engine 2.5T Turbo | $1 800 | 224 km/h | 118 |
| 3 | V6 3.0 | $3 500 | 246 km/h | 132 |
| 4 | V8 4.0 | $6 000 | 268 km/h | 146 |

### 🛞 Zawieszenie — wpływa na `grip` (przyczepność)

| Poziom | Nazwa | Koszt | Grip |
|---|---|---|---|
| 0 | Stock | — | 55% |
| 1 | Sport Springs | $600 | 64% |
| 2 | Coilovers | $1 400 | 73% |
| 3 | Racing Suspension | $2 800 | 82% |
| 4 | Full Aero Kit | $5 000 | 91% |

### ⚡ Nitro — wpływa na `nitroCapacity`

| Poziom | Nazwa | Koszt | Pojemność |
|---|---|---|---|
| 0 | Small Tank | — | 80 L |
| 1 | Medium Tank | $500 | 105 L |
| 2 | Large Tank | $1 200 | 130 L |
| 3 | Twin NOS | $2 400 | 155 L |
| 4 | Race NOS | $4 200 | 180 L |

### ⚙️ Skrzynia biegów

| Poziom | Nazwa | Koszt | Opis w UI |
|---|---|---|---|
| 0 | Sport 6-Speed | — | — |
| 1 | Sequential Racing | $1 000 | Shift speed +10% |
| 2 | Full Race Gearbox | $2 500 | Shift speed +25%, no lag |

### 🎨 Malowanie

8 dostępnych skinów Formuły (1 darmowy domyślny, kolejne za $250 każdy — zmiana wyłącznie wizualna).

---

## 📝 Status commitów i milestone'ów

| Wersja | Opis | Milestone |
|---|---|---|
| `v1.0.1` | Inicjalizacja projektu, system zapisu (`SaveSystem`), README.md | — |
| `v1.0.2` | Fizyka pojazdu (przyspieszenie, biegi, RPM, poślizg, nitro) | 🏷️ `v0.1-core-physics` |
| `v1.0.3` | Implementacja HUD (prędkościomierz, nitro, czas, pozycja) | — |
| `v1.0.4` | Implementacja zbieranek (monety, nitro, respawn) | — |
| `v1.0.5` | Implementacja garażu (ulepszenia, skiny, system kosztów) | 🏷️ `v0.2-systems-ready` |
| `v1.0.6` | Mapy i generowanie toru (Drag, Circuit) | 🏷️ `v0.3-world-and-ai`  |
| `v1.0.7` | Boty AI — jazda po waypointach, poziomy trudności | — |
| `v1.0.8` | Particle System (dym z opon, spaliny, nitro, iskry, zbieranie monet) | - |
| `v1.0.9` | Tryby gry: Circuit, Drag, Time Attack |  |
| `v1.1.0` | Integracja Game Loop, menu, ekran wyników, rekordy — finalna wersja | 🏷️ `v1.0-final-release` |

---

## 📁 Struktura katalogów

```text
NitroDrive/
│
├── assets/
│   ├── cars/          # Tekstury pojazdów (skiny gracza i botów)
│   ├── fonts/         # Czcionka HUD/menu
│   └── maps/          # Tekstury torów (Drag, Circuit)
│
├── src/
│   ├── main.cpp                  # Punkt wejścia
│   ├── Game.hpp / Game.cpp       # Główna pętla gry i stany (menu, garaż, wyniki...)
│   ├── Car.hpp / Car.cpp         # Model fizyki pojazdu
│   ├── Bot.hpp / Bot.cpp         # Logika AI botów
│   ├── Map.hpp / Map.cpp         # Generowanie i renderowanie torów
│   ├── HUD.hpp / HUD.cpp         # Interfejs gracza podczas wyścigu
│   ├── Collectible.hpp / .cpp    # Monety i nitro na trasie
│   ├── ParticleSystem.hpp / .cpp # Efekty cząsteczkowe
│   ├── Garage.hpp / .cpp         # Ekran garażu i system ulepszeń
│   ├── SaveSystem.hpp / .cpp     # Zapis/odczyt postępu gracza
│   ├── GameMode.hpp              # Interfejs trybu gry
│   ├── DragMode.hpp / .cpp       # Tryb Drag Race
│   ├── CircuitMode.hpp / .cpp    # Tryb Circuit
│   ├── TimeAttackMode.hpp / .cpp # Tryb Time Attack
│   └── Difficulty.hpp            # Poziomy trudności AI
│
├── CMakeLists.txt
└── README.md
```