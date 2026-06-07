#pragma once
#include <SFML/Graphics.hpp>
// Typy przedmiotow na mapie
enum class CollectibleType { COIN, NITRO };

class Collectible {
public:
// Tworzymy znajdzke podajac jej typ i pozycje startowa
    Collectible(CollectibleType type, sf::Vector2f pos,
                const sf::Texture& coinTex, const sf::Texture& nitroTex);

    void update(float dt);// Aktualizacja logiki (np. animacja lewitowania)
    void draw(sf::RenderTarget& target);// Rysowanie na ekranie
    bool isCollected() const { return m_collected; }// Sprawdza, czy auto najechalo na przedmiot
    CollectibleType getType() const { return m_type; }// Gettery do obslugi w glownej petli gry
    sf::FloatRect getBounds() const;
    void collect();

    // Respawn after delay
    static constexpr float RESPAWN_TIME = 5.f;

private:
    CollectibleType m_type;
    sf::Sprite      m_sprite;
    float           m_angle   = 0.f;
    float           m_pulse   = 0.f;
    bool            m_collected = false;
    float           m_respawnTimer = 0.f;
    sf::Vector2f    m_basePos;
};
