// Bot.cpp — Sztuczna inteligencja bota (pojazd sterowany przez komputer)
// Dziedziczy po Car; nadpisuje sterowanie poprzez updateAI() zamiast recznego
// wejscia uzytkownika.

#include "Bot.hpp"
#include <cmath>
#include <cstdlib>

// Stala PI uzywana do konwersji katow
static const float PI = 3.14159265f;

// Inicjalizuje baze Car (tekstura) i ustawia kolor nakladki odrozniajacej boty.
Bot::Bot(const sf::Texture& tex, sf::Color col) : Car(tex) {
    m_sprite.setColor(col);
}

// ── setWaypoints
// Przekazuje botowi liste punktow kontrolnych trasy (waypoints).
// Bot zawsze jedzie w kierunku aktualnego waypointu i automatycznie przechodzi
// do nastepnego po zblizeniu sie na dystans ponizej 90 px.
void Bot::setWaypoints(const std::vector<sf::Vector2f>& wps) {
    m_wps   = wps;
    m_wpIdx = 0;  // zacznij od poczatku trasy
}

// ── setDifficulty
// Dostosowuje parametry AI do wybranego poziomu trudnosci:
//   EASY   — wolniejszy, wiecej odchylenia od linii toru, dlugie opoznienie zmiany biegow
//   MEDIUM — zbalansowany
//   HARD   — bliski maksimum predkosci, minimalne odchylenie, szybkie zmiany biegow
void Bot::setDifficulty(Difficulty d) {
    m_diff = d;
    switch(d) {
    case Difficulty::EASY:
        // Bot jedzie na 68% maksymalnej mocy, duze bledy linii toru
        m_speedFactor = 0.68f;
        m_deviation   = 22.f;
        m_gearDelay   = 0.6f;
        break;
    case Difficulty::MEDIUM:
        // Zbalansowane parametry — dobry kompromis miedzy trudnoscia a zabawa
        m_speedFactor = 0.82f;
        m_deviation   = 10.f;
        m_gearDelay   = 0.15f;
        break;
    case Difficulty::HARD:
        // Bot jedzie na 94% mocy, prawie idealna linia toru
        m_speedFactor = 0.94f;
        m_deviation   = 4.f;
        m_gearDelay   = 0.05f;
        break;
    }
}

// ── getProgress
// Zwraca skalar postepy bota na trasie: im wiekszy, tym bot dalej.
// Wzor: okrazenia * liczba_waypointow + aktualny_waypoint.
// Uzywane przez CircuitMode::calcPos() do ustalania pozycji gracza w wyscgu.
float Bot::getProgress() const {
    if(m_wps.empty()) return 0.f;
    // Ta sama formula co gracz: m_lap odpowiada m_lap gracza (bot startuje od 0, gracz od 1)
    // wiec dodajemy 1 zeby byc w tej samej skali co (m_lap_gracza - 1)*total + wpIdx
    int total = (int)m_wps.size();
    int wpLocal = m_wpIdx % total;  // pozycja w biezacym okrazeniu
    return (float)(m_lap * total + wpLocal);
}

// ── autoShift
// Automatyczna zmiana biegow bota z opoznieniem (m_gearDelay).
// Wyzszy bieg przy >= 90% progu; nizszy bieg przy < 50% dolnego progu.
// Opoznienie zapobiega "dryganiu" bota przy skretach
void Bot::autoShift(float dt) {
    m_gearTimer += dt;
    if(m_gearTimer < m_gearDelay) return;  // jeszcze nie czas na zmiane
    m_gearTimer = 0.f;

    int maxG = getMaxGear();
    // Wbij wyzszy bieg gdy predkosc przekracza 90% maksimum dla danego biegu
    if(m_gear < maxG && m_speed >= GEAR_MAX[m_gear] * 0.90f) {
        m_gear++;
        m_rpm = std::max(1500.f, m_rpm * 0.55f);  // obrotomierz spada po zmianie
    }
    // Zejdz nizej gdy predkosc jest za mala dla aktualnego biegu
    else if(m_gear > 1 && m_speed < GEAR_MAX[m_gear - 1] * 0.5f) {
        m_gear--;
        m_rpm = std::min(8000.f, m_rpm * 1.5f);   // obrotomierz rosnie przy redukcji
    }
}

// ── updateAI
// Glowna funkcja AI — wywolywana co klatke zamiast recznego wejscia.
// Algorytm:
//   1. Wyznacz aktualny waypoint docelowy (z deterministycznym odchyleniem)
//   2. Oblicz kat do waypointu i uchylenie od kursu bota (delta)
//   3. Dostosuj predkosc i hamowanie zaleznie od ostrosci zakretu
//   4. Aktywuj nitro na prostych (tylko MEDIUM i HARD)
//   5. Wywolaj autoShift i bazowe update() fizyki (Car::update)
void Bot::updateAI(float dt) {
    // Bot ukonczyl swoje okrazenia — stoi w miejscu, nie aktualizuj AI
    if(m_finished) return;

    // Jesli brak waypointow (np. tryb niezainicjowany), jedz prosto
    if(m_wps.empty()) { update(dt); return; }

    int total = (int)m_wps.size();
    sf::Vector2f pos = m_sprite.getPosition();

    // ── Krok 1: Przejdz przez bliskie waypointy (dystans < 90 px)
    for(int guard = 0; guard < total; guard++) {
        int idx = m_wpIdx % total;
        sf::Vector2f diff = m_wps[idx] - pos;
        if(std::hypot(diff.x, diff.y) >= 90.f) break;

        m_wpIdx++;

        // Sprawdz czy bot przekroczyl kolejne okrazenie
        if(m_wpIdx % total == 0) {
            if(m_isDrag) {
                m_wpIdx = total - 1;
                break;
            } else {
                m_lap++;
                if(m_lap >= m_maxLaps) {
                    m_finished = true;
                    m_throttle = 0.f;
                    m_brake    = 1.f;
                    update(dt);
                    return;
                }
                // Znajdz najblizszy waypoint ktory jest PRZED botem (nie za nim)
                // aby uniknac natychmiastowego zaliczenia wp[0] i petli
                int bestIdx = 0;
                float bestDist = 9999.f;
                // Szukaj w pierwszej cwierci trasy waypointu dalszego niz 150px
                for(int k = 1; k < total/4; k++) {
                    float d = std::hypot(m_wps[k].x - pos.x, m_wps[k].y - pos.y);
                    if(d > 150.f && d < bestDist) {
                        bestDist = d;
                        bestIdx  = k;
                        break;  // pierwszy pasujacy wystarczy
                    }
                }
                // Ustaw m_wpIdx tak zeby % total wskazywal na znaleziony wp
                m_wpIdx = (m_wpIdx / total) * total + bestIdx;
                break;
            }
        }
    }

    // ── Krok 2: Wyznacz punkt docelowy z deterministycznym szumem
    int idx = m_wpIdx % total;
    sf::Vector2f wp = m_wps[idx];

    if(!m_isDrag) {
        float devPhase = (float)(m_botId * 41 + idx * 17 + m_lap * 11);
        wp.x += m_deviation * std::sin(devPhase * 0.37f);
        wp.y += m_deviation * std::cos(devPhase * 0.53f);
    }

    // ── Krok 3: Oblicz kat i uchylenie kursowe
    sf::Vector2f diff = wp - pos;
    float targetAngle = std::atan2(diff.y, diff.x) * 180.f / PI;
    float delta = targetAngle - m_angle;
    while(delta >  180.f) delta -= 360.f;
    while(delta < -180.f) delta += 360.f;

    // ── Krok 3b: Waypoint za plecami bota (np. po kolizji)
    if(std::abs(delta) > 130.f && !m_isDrag) {
        m_wpIdx++;
        if(m_wpIdx % total == 0) {
            m_lap++;
            if(m_lap >= m_maxLaps) {
                m_finished = true;
                m_throttle = 0.f;
                m_brake    = 1.f;
                update(dt);
                return;
            }
            // Znajdz najblizszy waypoint ktory jest PRZED botem (nie za nim)
            int bestIdx = 0;
            float bestDist = 9999.f;
            for(int k = 1; k < total/4; k++) {
                float d = std::hypot(m_wps[k].x - pos.x, m_wps[k].y - pos.y);
                if(d > 150.f && d < bestDist) {
                    bestDist = d;
                    bestIdx  = k;
                    break;
                }
            }
            m_wpIdx = (m_wpIdx / total) * total + bestIdx;
        }
        idx = m_wpIdx % total;
        wp  = m_wps[idx];
        float devPhase = (float)(m_botId * 41 + idx * 17 + m_lap * 11);
        wp.x += m_deviation * std::sin(devPhase * 0.37f);
        wp.y += m_deviation * std::cos(devPhase * 0.53f);
        diff        = wp - pos;
        targetAngle = std::atan2(diff.y, diff.x) * 180.f / PI;
        delta       = targetAngle - m_angle;
        while(delta >  180.f) delta -= 360.f;
        while(delta < -180.f) delta += 360.f;
    }

    // W trybie drag ograniczamy skrecanie (prosty tor, boty jada prosto)
    if(m_isDrag) delta = std::clamp(delta, -20.f, 20.f);

    float absDelta = std::abs(delta);

    // ── Krok 4: Bezposrednia korekta kata (bez fizyki skrecania)
    // Bot obraca sie maksymalnie o maxTurn stopni na klatke
    float maxTurn = 220.f * dt;
    if(delta >  maxTurn) m_angle += maxTurn;
    else if(delta < -maxTurn) m_angle -= maxTurn;
    else m_angle += delta;
    m_steering = 0.f;  // brak fizycznego sterowania kolami — kat jest nadpisywany

    // ── Krok 5: Throttle i hamowanie zaleznie od ostrosci zakretu
    // Im ostrzejszy zakret, tym mniejszy gaz i ewentualne hamowanie
    float cornerFactor;
    if     (absDelta >  90.f) { cornerFactor = 0.25f; m_brake = 0.3f; }  // ostry zakret
    else if(absDelta >  55.f) { cornerFactor = 0.55f; m_brake = 0.f; }   // sredni zakret
    else if(absDelta >  30.f) { cornerFactor = 0.80f; m_brake = 0.f; }   // lagodny zakret
    else                      { cornerFactor = 1.f;   m_brake = 0.f; }   // prosta

    m_throttle  = m_speedFactor * cornerFactor;  // gaz skalowany przez trudnosc
    m_handbrake = false;

    // ── Krok 6: Nitro na prostej (HARD i MEDIUM z zapasem)
    if(m_diff == Difficulty::HARD   && m_nitro > 35.f && absDelta < 10.f) activateNitro();
    if(m_diff == Difficulty::MEDIUM && m_nitro > 55.f && absDelta <  8.f) activateNitro();

    // ── Krok 7: Zmiana biegow i aktualizacja fizyki (Car::update)
    autoShift(dt);
    update(dt);  // wywoluje bazowa fizyke z Car::update
}