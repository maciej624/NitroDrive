#include "ParticleSystem.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static const float PI = 3.14159265f;

// Losuje float z zadanego przedziału
static float randF(float lo, float hi) {
    return lo + (float)(rand()) / RAND_MAX * (hi - lo);
}

ParticleSystem::ParticleSystem() {}

// Główny emiter - odpala konkretny efekt w zależności od typu
void ParticleSystem::emit(ParticleEffect effect, sf::Vector2f pos, float angle, float intensity) {
    if (m_particles.size() >= MAX_PARTICLES) return;

    switch (effect) {
    case ParticleEffect::SMOKE_TIRE:   emitSmokeTire(pos, angle, intensity); break;
    case ParticleEffect::EXHAUST:      emitExhaust(pos, angle, intensity);   break;
    case ParticleEffect::NITRO_FLAME:  emitNitroFlame(pos, angle, intensity);break;
    case ParticleEffect::SPARK:        emitSpark(pos);                       break;
    case ParticleEffect::COIN_COLLECT: emitCoinCollect(pos);                 break;
    }
}

// Zostawia ślad opon na asfalcie
void ParticleSystem::addTireMark(sf::Vector2f pos, float angle, float width) {
    // Usuwamy najstarszy ślad, jeśli brakuje już miejsca
    if (m_tireMarks.size() >= MAX_TIRE_MARKS) m_tireMarks.pop_front();

    TireMark m;
    m.shape.setSize({width, 8.f});
    m.shape.setOrigin(width / 2.f, 4.f);
    m.shape.setPosition(pos);
    m.shape.setRotation(angle);
    m.shape.setFillColor(sf::Color(20, 20, 20, 180));

    m_tireMarks.push_back(m);
}

// Aktualizacja fizyki i życia cząsteczek
void ParticleSystem::update(float dt) {
    for (auto& p : m_particles) {
        p.lifetime -= dt;
        p.position += p.velocity * dt;
        p.velocity *= 0.97f; // Powolne wyhamowywanie
        p.rotation += p.rotSpeed * dt;

        // Powiększanie cząsteczki z biegiem czasu
        float t = 1.f - p.lifetime / p.maxLifetime;
        p.radius = p.maxRadius * (0.5f + t * 0.5f);

        // Płynne zanikanie (zmniejszanie kanału alpha)
        uint8_t alpha = (uint8_t)(255 * (p.lifetime / p.maxLifetime));
        p.color.a = alpha;
    }

    // Wyrzucenie z wektora martwych cząsteczek
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p){ return p.lifetime <= 0.f; }),
        m_particles.end());
}

// Rysowanie wszystkiego na ekranie
void ParticleSystem::draw(sf::RenderTarget& target) {
    // Ślady opon rysujemy najpierw, żeby były pod spodem
    for (auto& m : m_tireMarks) {
        target.draw(m.shape);
    }

    // Cząsteczki rysujemy na wierzchu
    sf::CircleShape circle;
    for (auto& p : m_particles) {
        circle.setRadius(p.radius);
        circle.setOrigin(p.radius, p.radius);
        circle.setPosition(p.position);
        circle.setFillColor(p.color);
        circle.setRotation(p.rotation);
        target.draw(circle);
    }
}

// Reset całego systemu
void ParticleSystem::clear() {
    m_particles.clear();
    m_tireMarks.clear();
}

// Generowanie dymu przy paleniu gumy lub poślizgu
void ParticleSystem::emitSmokeTire(sf::Vector2f pos, float angle, float intensity) {
    int count = (int)(6 * intensity);
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = pos;
        float spread = randF(-PI/4.f, PI/4.f);
        float speed  = randF(10.f, 50.f);
        float rad    = (angle + 180.f + spread * 57.3f) * PI / 180.f;

        p.velocity    = {std::cos(rad)*speed, std::sin(rad)*speed};
        p.maxLifetime = p.lifetime = randF(0.6f, 1.4f);
        p.maxRadius   = p.radius  = randF(8.f, 20.f);

        // Odcienie szarego dla dymu
        uint8_t g = (uint8_t)randF(180, 230);
        p.color       = {g, g, g, 200};
        p.rotSpeed    = randF(-60.f, 60.f);
        p.rotation    = randF(0.f, 360.f);

        m_particles.push_back(p);
    }
}

// Ciemne spaliny z wydechu
void ParticleSystem::emitExhaust(sf::Vector2f pos, float angle, float intensity) {
    int count = (int)(3 * intensity);
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = pos;
        float rad  = (angle + 180.f + randF(-10.f, 10.f)) * PI / 180.f;
        float speed = randF(20.f, 40.f);

        p.velocity    = {std::cos(rad)*speed, std::sin(rad)*speed};
        p.maxLifetime = p.lifetime = randF(0.3f, 0.7f);
        p.maxRadius   = p.radius  = randF(4.f, 10.f);
        p.color       = {120, 120, 120, 160};
        p.rotSpeed    = 0.f;
        p.rotation    = 0.f;

        m_particles.push_back(p);
    }
}

// Płomień z nitro (niebiesko-biały)
void ParticleSystem::emitNitroFlame(sf::Vector2f pos, float angle, float intensity) {
    int count = (int)(8 * intensity);
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = pos;
        float spread = randF(-0.3f, 0.3f);
        float speed  = randF(80.f, 200.f);
        float rad    = (angle + 180.f) * PI / 180.f + spread;

        p.velocity    = {std::cos(rad)*speed, std::sin(rad)*speed};
        p.maxLifetime = p.lifetime = randF(0.1f, 0.3f);
        p.maxRadius   = p.radius  = randF(3.f, 8.f);

        uint8_t r = (uint8_t)randF(100, 200);
        uint8_t g = (uint8_t)randF(150, 255);
        uint8_t b = 255;
        p.color = {r, g, b, 220};
        p.rotSpeed = 0.f;
        p.rotation = 0.f;

        m_particles.push_back(p);
    }
}

// Iskry przy stłuczkach i otarciach
void ParticleSystem::emitSpark(sf::Vector2f pos) {
    for (int i = 0; i < 12; ++i) {
        Particle p;
        p.position = pos;
        float angle = randF(0.f, 2*PI);
        float speed = randF(60.f, 180.f);

        p.velocity    = {std::cos(angle)*speed, std::sin(angle)*speed};
        p.maxLifetime = p.lifetime = randF(0.15f, 0.4f);
        p.maxRadius   = p.radius  = 2.5f;

        // Pomarańczowo-żółte kolory
        uint8_t g = (uint8_t)randF(180, 255);
        p.color = {255, g, 0, 255};
        p.rotSpeed = 0.f;
        p.rotation = 0.f;

        m_particles.push_back(p);
    }
}

// Złoty rozbłysk po zebraniu monety
void ParticleSystem::emitCoinCollect(sf::Vector2f pos) {
    for (int i = 0; i < 10; ++i) {
        Particle p;
        p.position = pos;
        float angle = randF(0.f, 2*PI);
        float speed = randF(40.f, 120.f);

        p.velocity    = {std::cos(angle)*speed, std::sin(angle)*speed};
        p.maxLifetime = p.lifetime = randF(0.3f, 0.6f);
        p.maxRadius   = p.radius  = 4.f;
        p.color = {255, 220, 0, 255};
        p.rotSpeed = randF(-90.f, 90.f);
        p.rotation = 0.f;

        m_particles.push_back(p);
    }
}
