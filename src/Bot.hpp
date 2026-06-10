#pragma once
#include "Car.hpp"
#include "Difficulty.hpp"
#include <vector>

class Bot : public Car {
public:
    Bot(const sf::Texture& tex, sf::Color col);// Tworzy bota z podana tekstura i kolorem nakladki (odroznia boty na torze)
    void setWaypoints(const std::vector<sf::Vector2f>& wps);  // Ustawia liste punktow kontrolnych trasy
    void setDifficulty(Difficulty d);  // Dostosowuje predkosc, odchylenie i timing zmiany biegow do poziomu trudnosci
    void setBotId(int id) { m_botId=id; } // Ustawia unikalny identyfikator bota
    void setDragMode(bool b) { m_isDrag=b; }// Przelacza bot w tryb drag (jedzie prosto, ograniczone skrecanie)
    void resetProgress() { m_wpIdx=0; m_lap=0; m_finished=false; }    // Zeruje licznik waypointow, okrazen i flage ukonczenia wyscigu
    float getProgress() const;    // Zwraca skalar postepu na trasie: okrazenia * liczba_wp + aktualny_wp
    // Uzywane przez CircuitMode do obliczania pozycji gracza w wyscgu
    bool isFinished() const { return m_finished; }    // Zwraca true jesli bot ukonczyl wszystkie wymagane okrazenia
    void setMaxLaps(int laps) { m_maxLaps=laps; }    // Ustawia liczbe okrazen po ktorej bot zatrzymuje sie
    void updateAI(float dt);  // Glowna funkcja AI — wywolywana co klatke zamiast recznego wejscia
private:
    std::vector<sf::Vector2f> m_wps; //lista waypointsow
    int   m_wpIdx=0, m_lap=0, m_botId=0; //id waypointsu,okrazenia,i bot id 
    int   m_maxLaps=5; //limit okrazen
    bool  m_finished=false; //czy bot ukonczyl wyscig
    bool  m_isDrag=false; //czy drag
    float m_speedFactor=0.85f; //mnoznik predkosci zalezny od trudnosci
    float m_deviation=10.f;//odchykenie od idealnej prostej toru px
    float m_gearDelay=0.2f; //minimalny czas miedzy zmianami biegu
    float m_gearTimer=0.f; //licznik czasu od ostatniej zmiany biegu
    Difficulty m_diff=Difficulty::MEDIUM;
    void autoShift(float dt);  // Automatyczna zmiana biegow z opoznieniem
};
