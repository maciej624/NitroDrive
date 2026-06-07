#include "Collectible.hpp"
#include <cmath>

static const float PI = 3.14159265f;

Collectible::Collectible(CollectibleType type, sf::Vector2f pos,
                         const sf::Texture& coinTex, const sf::Texture& nitroTex)
    : m_type(type), m_basePos(pos)
{
    const sf::Texture& tex = (type == CollectibleType::COIN) ? coinTex : nitroTex;// Wybieramy odpowiednia teksture w zaleznosci od typu przedmiotu
    m_sprite.setTexture(tex);
    auto b = m_sprite.getLocalBounds();// Ustawiamy srodek obrotu (origin) dokladnie na srodek obrazka
    m_sprite.setOrigin(b.width / 2.f, b.height / 2.f);
    m_sprite.setPosition(pos);
}
// Jesli przedmiot jest zebrany, odliczamy czas do jego powrotu
void Collectible::update(float dt) {
    if(m_collected){
        m_respawnTimer += dt;
        if(m_respawnTimer >= RESPAWN_TIME){
            m_collected = false;
            m_respawnTimer = 0.f;
            m_sprite.setPosition(m_basePos);
        }
        return;
    }// Obracanie przedmiotu wokol wlasnej osi
    m_angle += 90.f * dt;
    if(m_angle > 360.f) m_angle -= 360.f;
    m_pulse += dt * 3.f;
// Efekt "bicia serca" - przedmiot lekko rosnie i maleje
    float scale = 1.f + 0.12f * std::sin(m_pulse);
    m_sprite.setScale(scale, scale);
    m_sprite.setRotation(m_angle);

    float bob = std::sin(m_pulse) * 3.f;
    m_sprite.setPosition(m_basePos.x, m_basePos.y + bob);
}

void Collectible::draw(sf::RenderTarget& target) {
    if(!m_collected) target.draw(m_sprite);
}
// Zwraca ramke kolizyjna, zeby mozna bylo wykryc zderzenie z autem
sf::FloatRect Collectible::getBounds() const {
    return m_sprite.getGlobalBounds();
}
// Oznacza przedmiot jako zebrany i resetuje licznik czasu powrotu
void Collectible::collect() {
    m_collected = true;
    m_respawnTimer = 0.f;
}
