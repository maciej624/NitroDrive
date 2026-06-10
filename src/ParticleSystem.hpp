#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>

enum class ParticleEffect {
    SMOKE_TIRE,
    EXHAUST,
    NITRO_FLAME,
    SPARK,
    COIN_COLLECT
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float        lifetime;
    float        maxLifetime;
    float        radius;
    float        maxRadius;
    sf::Color    color;
    float        rotation;
    float        rotSpeed;
};

struct TireMark {
    sf::RectangleShape shape;
};

class ParticleSystem {
public:
    ParticleSystem();

    void emit(ParticleEffect effect, sf::Vector2f pos, float angle, float intensity = 1.f);
    void addTireMark(sf::Vector2f pos, float angle, float width = 6.f);
    void update(float dt);
    void draw(sf::RenderTarget& target);
    void clear();

private:
    std::vector<Particle>  m_particles;
    std::deque<TireMark>   m_tireMarks;

    static constexpr size_t MAX_TIRE_MARKS = 2000;
    static constexpr size_t MAX_PARTICLES  = 1500;

    void emitSmokeTire(sf::Vector2f pos, float angle, float intensity);
    void emitExhaust(sf::Vector2f pos, float angle, float intensity);
    void emitNitroFlame(sf::Vector2f pos, float angle, float intensity);
    void emitSpark(sf::Vector2f pos);
    void emitCoinCollect(sf::Vector2f pos);
};
