#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>

struct CarStats {
    float maxSpeed      = 200.f;
    float acceleration  = 100.f;
    float grip          = 0.75f;
    float nitroCapacity = 100.f;
    int   skinIndex     = 0;
};

// Klasa Car - Serce gry
// Odpowiada za to, zeby auto nie tylko latalo po ekranie, ale tez 
// zachowywalo sie jak samochod 

class Car {
public:
    Car() = default;
    explicit Car(const sf::Texture& tex);
    virtual ~Car() = default;

    sf::Vector2f  getPosition()    const { return m_sprite.getPosition(); }
    float         getAngle()       const { return m_angle; }
    float         getSpeed()       const { return m_speed; }
    float         getRPM()         const { return m_rpm; }
    int           getGear()        const { return m_gear; }
    float         getNitro()       const { return m_nitro; }
    float         getNitroMax()    const { return m_stats.nitroCapacity; }
    bool          isSliding()      const { return m_sliding; }
    float         getSlideAngle()  const { return m_slideAngle; }

    bool          isDrifting()     const { return m_sliding; }
    float         getDriftAngle()  const { return m_slideAngle; }
    bool          isNitroActive()  const { return m_nitroActive; }
    sf::FloatRect getBounds()      const { return m_sprite.getGlobalBounds(); }
    sf::Sprite&   getSprite()            { return m_sprite; }

    void setPosition(sf::Vector2f p)  { m_sprite.setPosition(p); }
    void setAngle(float deg)          { m_angle = deg; }
    float getShiftCooldown() const    { return m_shiftCooldown; }
    int   getMaxGear()        const;
    // Bieg max zalezy od maxSpeed silnika:
    // eng0-2 (maxSpeed 180-224): biegi 1-5
    // eng3-5 (maxSpeed 246-290): biegi 1-6
    void setThrottle(float t)         { m_throttle = std::clamp(t, 0.f,1.f); }
    void setBrake(float b)            { m_brake    = std::clamp(b, 0.f,1.f); }
    void setSteering(float s)         { m_steering = std::clamp(s,-1.f,1.f); }
    void setHandbrake(bool h)         { m_handbrake = h; }
    void applyStats(const CarStats& s){ m_stats = s; }
    // Zmiany biegow , jest cooldown zeby nie dalo sie wrzucic 6 przy starcie 
    void shiftUp();
    void shiftDown();

    void activateNitro();
    void addNitro(float v);
    void applyCollisionImpact(float otherSpeed);  // spowalnia na kolizji
    void update(float dt); // glowna petla fizyki, jezeli zachowuje sie jak w kosmosie to pewnie trzeba pozmieniac m_speed i deltaTime
    void draw(sf::RenderTarget& t);
    void reset();

protected:
    sf::Sprite m_sprite;
    float m_angle=0,m_speed=0,m_rpm=800,m_nitro=0,m_slideAngle=0;
    int   m_gear=1;
    bool  m_sliding=false,m_nitroActive=false;
    float m_throttle=0,m_brake=0,m_steering=0;
    bool  m_handbrake=false;
    CarStats m_stats;
    float m_collisionTimer=0.f;  // cooldown to avoid repeated collision hits
    float m_shiftCooldown=0.f;   // cooldown between manual gear shifts

    static constexpr float GEAR_MAX[7]={0,40,80,120,160,200,250};
};
