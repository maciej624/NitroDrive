#include "Car.hpp"

static const float PI=3.14159265f;
static const float D2R=PI/180.f;
constexpr float Car::GEAR_MAX[7];

Car::Car(const sf::Texture& tex){
    m_sprite.setTexture(tex);
    auto b=m_sprite.getLocalBounds();
    m_sprite.setOrigin(b.width/2.f,b.height/2.f);
    if(b.width>0 && b.width<=64.f)
        m_sprite.setScale(1.6f,1.6f);
}

int Car::getMaxGear() const {
    //szukamy biegu, ktory pasuje do mocy silnika.
    for(int g=6; g>=1; g--){
        float mid = (GEAR_MAX[g] + GEAR_MAX[g>0?g-1:0]) * 0.5f;
        if(m_stats.maxSpeed >= mid) return g;
    }
    return 1;
}

void Car::shiftUp(){
    if(m_gear < getMaxGear()){
        m_gear++;
        m_rpm=std::max(1500.f,m_rpm*0.55f);
        m_shiftCooldown=0.6f; // Blokada, zeby nie zmieniac biegow co klatke
    }
}
void Car::shiftDown(){
    if(m_gear>1){
        m_gear--;
        m_rpm=std::min(8000.f,m_rpm*1.5f);
        m_shiftCooldown=0.4f; // cooldown po zmianie biegu w dol
    }
}
void Car::activateNitro(){
    if(m_nitro>5.f) m_nitroActive=true;
}
void Car::addNitro(float v){
    m_nitro=std::min(m_nitro+v,m_stats.nitroCapacity);
}

void Car::update(float dt){

    if(m_collisionTimer>0.f) m_collisionTimer-=dt;
    if(m_shiftCooldown>0.f)   m_shiftCooldown-=dt;

    //daje predkosc ale sie zuzywa 
    float nitroBoost=1.f;
    if(m_nitroActive){
        m_nitro-=dt*25.f;  
        if(m_nitro<=0.f){m_nitro=0.f;m_nitroActive=false;}
        else nitroBoost=1.35f;
    }

    float gearTop = GEAR_MAX[m_gear];
    float accel   = m_stats.acceleration * m_throttle * nitroBoost;
    float drag    = (m_speed>0.1f) ? (3.f + m_speed*m_speed*0.002f) : 0.f;
    float braking = m_brake * 250.f;

    m_speed += (accel - drag - braking) * dt;
    //ograniczenia predkosci zalezne od silnika lub biegu
    float softCap = std::min(m_stats.maxSpeed, gearTop);
    m_speed = std::clamp(m_speed, 0.f, softCap);


    {
        //rpm 
        float lo = GEAR_MAX[m_gear-1<1?0:m_gear-1];
        float hi = GEAR_MAX[m_gear];
        float range = hi - lo;
        if(range > 0.f)
            m_rpm = 1000.f + std::clamp((m_speed - lo) / range, 0.f, 1.f) * 7000.f;
        else
            m_rpm = 800.f + m_throttle*2000.f;
        m_rpm = std::clamp(m_rpm, 800.f, 8500.f);
    }

    //przy duzej predkosci ciezej skrecic
    float steerFactor = std::clamp(m_speed/30.f, 0.1f, 1.f);
    if(m_speed>80.f) steerFactor *= std::max(0.4f, 80.f/m_speed);
    float steerDeg = 100.f * m_steering * steerFactor * dt;


    bool wantSlide = m_handbrake && m_speed > 20.f;
    if(wantSlide && !m_sliding){
        m_sliding = true;
        m_slideAngle = m_steering * 30.f;
    }
    if(!m_handbrake || m_speed < 10.f){
        m_sliding = false;
    }

    if(m_sliding){
        steerDeg = 140.f * m_steering * std::clamp(m_speed/25.f,0.1f,1.f) * dt;
        float slideTarget = m_steering * 50.f * (1.f - m_stats.grip * 0.5f);
        float slideRate = 3.5f;
        m_slideAngle += (slideTarget - m_slideAngle) * slideRate * dt;
        m_speed *= (1.f - dt * 0.6f * std::abs(m_slideAngle) / 50.f);
    } else {
        m_slideAngle += (0.f - m_slideAngle) * 7.f * dt;
        if(std::abs(m_slideAngle) < 0.3f) m_slideAngle = 0.f;
    }

    m_angle += steerDeg;


    float pxPerSec = m_speed * 1.65f;
    float moveAngle = (m_angle + m_slideAngle * 0.55f) * D2R;

    auto pos=m_sprite.getPosition();
    pos.x += std::cos(moveAngle)*pxPerSec*dt;
    pos.y += std::sin(moveAngle)*pxPerSec*dt;
    m_sprite.setPosition(pos);
    m_sprite.setRotation(m_angle + m_slideAngle * 0.45f + 90.f);
}

void Car::draw(sf::RenderTarget& t){ t.draw(m_sprite); }

void Car::reset(){
    m_speed=0;m_rpm=800;m_gear=1;m_nitro=0;
    m_sliding=false;m_slideAngle=0;m_nitroActive=false;
    m_throttle=0;m_brake=0;m_steering=0;m_handbrake=false;
}

void Car::applyCollisionImpact(float otherSpeed){
    if(m_collisionTimer>0.f) return;
    m_collisionTimer=0.4f; // nie dostajemy kolizji co klatke 
    float speedDiff=std::abs(m_speed - otherSpeed);
    float lossFactor=0.25f + std::min(0.35f, speedDiff/200.f);
    m_speed *= (1.f - lossFactor);
    m_nitroActive=false;
    if(m_gear>1 && m_speed < GEAR_MAX[m_gear-1]*0.6f) shiftDown();
}
