#pragma once

// Ustawienia wyścigów
static constexpr int CIRCUIT_LAPS     = 5;  // liczba okrążeń dla trybu Circuit
static constexpr int TIME_ATTACK_LAPS = 3;  // liczba okrążeń dla Time Attack

#include <SFML/Graphics.hpp>
#include <memory>
#include <array>
#include "Car.hpp"
#include "Bot.hpp"
#include "DragMode.hpp"
#include "CircuitMode.hpp"
#include "TimeAttackMode.hpp"
#include "Garage.hpp"
#include "SaveSystem.hpp"

// Dostępne ekrany i stany w grze
enum class GS { MENU, MODE, DIFF, COUNTDOWN, RACE, PAUSE, RESULTS, GARAGE, RECORDS };

class Game {
public:
    Game();
    void run(); // Główna pętla gry

private:
    // Okno i czas
    sf::RenderWindow m_win;
    sf::Clock        m_clk;
    bool             m_fs=false; // Flaga pełnego ekranu

    // Stan gry
    GS               m_state=GS::MENU;
    int              m_mode=0;  // Wybrany tryb: 0=DRAG, 1=CIRCUIT, 2=TIME ATTACK
    Difficulty       m_diff=Difficulty::MEDIUM; // Poziom trudności

    // Zasoby
    sf::Font    m_font;
    sf::Texture m_playerTex, m_botTex[4];
    std::array<sf::Texture,8> m_skinTex;
    sf::Texture m_garageBg;

    // Zarządzanie pojazdami
    std::unique_ptr<Car>  m_playerOwned;          // Obiekt auta gracza
    std::vector<std::unique_ptr<Bot>> m_bots;     // Obiekty botów

    // Wspólna lista wszystkich aut na torze (gracz na indeksie 0, reszta to boty)
    // Pozwala na łatwiejsze aktualizowanie i rysowanie całej stawki
    std::vector<Car*> m_cars;

    Car* m_player = nullptr; // Wskaźnik pomocniczy na auto gracza

    // Moduły trybów gry
    std::unique_ptr<DragMode>       m_drag;
    std::unique_ptr<CircuitMode>    m_circuit;
    std::unique_ptr<TimeAttackMode> m_timeAttack;
    GameMode* m_active=nullptr; // Wskaźnik na obecnie ogrywany tryb

    // Systemy poboczne
    Garage     m_garage;
    SaveSystem m_save;

    // Zmienne pomocnicze dla wyścigu i interfejsu
    float m_cdTimer=0; // Czas odliczania przed startem
    bool  m_cdDone=false;
    RaceResult m_lastResult; // Wynik ostatniego wyścigu
    bool  m_confirmReset=false; // Flaga potwierdzenia usunięcia zapisu

    float m_menuTime=0.f; // Czas dla animacji w menu

    sf::Vector2f m_mouse; // Pozycja kursora
    bool         m_click=false; // Czy kliknięto lewy przycisk myszy

    // Główne funkcje logiki
    void processEvents(); // Obsługa klawiatury i myszy
    void update(float dt); // Aktualizacja mechaniki
    void render(); // Rysowanie klatek
    void startRace(); // Inicjalizacja wyścigu
    void toggleFs(); // Przełączanie pełnego ekranu
    void buildAssets(); // Ładowanie tekstur i czcionek

    // Funkcje narzędziowe interfejsu
    sf::Text T(const std::string& s,unsigned sz,sf::Color c=sf::Color::White); // Generowanie tekstu
    bool     Btn(sf::RenderTarget& t,float x,float y,float w,float h,
             const std::string& lbl,bool sel=false); // Rysowanie przycisku i sprawdzanie kliknięcia
    void     drawBg(sf::RenderTarget& t); // Rysowanie animowanego tła menu

    // Funkcje rysujące poszczególne ekrany
    void drawMenu();
    void drawMode();
    void drawDiff();
    void drawCountdown();
    void drawPause();
    void drawResults();
    void drawRecords();

    static std::string fmtT(float s); // Formatowanie sekund do postaci MM:SS.ss
};