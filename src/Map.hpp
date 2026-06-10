#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// dostępne tryby gry i trasy
enum class MapType { DRAG, DRIFT, CIRCUIT };

class Map {
public:
    Map();

    // resetuje i buduje wybraną mapę od nowa
    void generate(MapType type, unsigned seed=42);

    // wyświetla tło i całą geometrię toru
    void draw(sf::RenderTarget& t, sf::RenderStates s=sf::RenderStates::Default);

    // podstawowe gettery do pobierania danych mapy
    const std::vector<sf::Vector2f>& getWaypoints() const { return m_waypoints; }
    const std::vector<sf::Vector2f>& getLaneWaypoints(int ln) const { return m_laneWaypoints[ln]; }
    const std::vector<sf::Vector2f>& getStartPositions() const { return m_startPositions; }
    sf::Vector2f getSize() const { return m_size; }
    sf::FloatRect getBounds() const { return m_bounds; }
    MapType getType() const { return m_type; }

    // sprawdza czy obiekt znajduje się na twardej nawierzchni
    bool isOnTrack(sf::Vector2f pos) const;

    // prostokąt wyznaczający pole startu i mety
    sf::FloatRect getFinishZone() const { return m_finishZone; }

private:
    MapType m_type = MapType::DRAG;

    // tekstura tła
    sf::Texture m_bgTex;
    sf::Sprite m_bgSpr;
    bool m_bgLoaded = false;

    // wymiary całej przestrzeni mapy
    sf::Vector2f m_size = {1280, 720};
    sf::FloatRect m_bounds = {0, 0, 1280, 720};
    sf::FloatRect m_finishZone = {0, 0, 1, 1};

    // zbiór wszystkich wierzchołków do rysowania trasy
    std::vector<sf::Vertex> m_v;

    // punkty ułatwiające botom jazdę oraz miejsca startu
    std::vector<sf::Vector2f> m_waypoints, m_startPositions;

    // trasy przypisane do konkretnych pasów (przydatne przy drag)
    std::vector<sf::Vector2f> m_laneWaypoints[5];

    // fizyczne odcinki toru używane do kolizji z trawnikiem
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> m_trackSegs;
    float m_trackHW = 52.f; // bazowa szerokość od środka asfaltu

    // narzędzia do łatwego tworzenia kształtów (czworokąty, prostokąty, segmenty)
    void addQ(std::vector<sf::Vertex>&, sf::Vector2f, sf::Vector2f, sf::Vector2f, sf::Vector2f, sf::Color);
    void addR(std::vector<sf::Vertex>&, sf::FloatRect, sf::Color);
    void addSeg(std::vector<sf::Vertex>&, sf::Vector2f, sf::Vector2f, float, sf::Color);

    // generatory dla poszczególnych rodzajów wyścigów
    void generateDrag();
    void generateDrift();
    void generateCircuit();
};
