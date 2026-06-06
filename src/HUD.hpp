#pragma once
#include <SFML/Graphics.hpp>
#include <string>
// Struktura przechowujaca wszystkie dane potrzebne do wyswietlenia interfejsu
struct HUDData {
    float speed=0, rpm=0, nitro=0, nitroMax=100;
    int   gear=1, coins=0, lap=1, totalLaps=5;
    int   position=1, totalCars=5;
    float raceTime=0, lapTime=0, bestLap=0;
    int   driftScore=0, combo=0;
    float driftAngle=0;
    bool  showRPM=false, showDrift=false, showLap=false;
    bool  offTrack=false;
};

class HUD {
public:
    // Laduje czcionke z pliku. Zwraca true jesli sie udalo
    bool loadFont(const std::string& path);
    // Glowna funkcja rysujaca wszystkie elementy HUD-a na ekranie
    void draw(sf::RenderTarget& t, const HUDData& d, sf::Vector2u ws);
private:
    sf::Font m_font;
    bool     m_ok=false; // Czy czcionka jest poprawnie zaladowana
    // Funkcje pomocnicze do rysowania poszczegolnych elementow
    sf::Text txt(const std::string& s, unsigned sz,
                 sf::Color c=sf::Color::White);
    // Rysuje tlo panelu z obramowaniem             
    void panel(sf::RenderTarget& t, float x, float y, float w, float h,
               sf::Color fill={0,0,0,160}, sf::Color outline={60,80,140,220});
    // Skladowe funkcje rysujace rozne czesci licznika
    void drawSpeedo(sf::RenderTarget& t, const HUDData& d, sf::Vector2u ws);
    void drawNitroBar(sf::RenderTarget& t, const HUDData& d, sf::Vector2u ws);
    void drawTopCenter(sf::RenderTarget& t, const HUDData& d, sf::Vector2u ws);
    void drawGear(sf::RenderTarget& t, const HUDData& d, sf::Vector2u ws);
    //formatowania czasu wyscigu (MM:SS.ms)
    static std::string fmtT(float s);
};
