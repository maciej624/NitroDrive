#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "Car.hpp"
#include "Bot.hpp"
#include "Collectible.hpp"
#include "ParticleSystem.hpp"

struct RaceResult {
    float totalTime=0,bestTime=0;
    int   driftScore=0,coinsEarned=0,position=1;
    bool  finished=false;
};

class GameMode {
public:
    virtual ~GameMode()=default;
    virtual void init(Car* player,
                      std::vector<std::unique_ptr<Bot>>& bots)=0;
    virtual void handleInput(const sf::Event& e)=0;
    virtual void update(float dt)=0;
    virtual void draw(sf::RenderTarget& target)=0;
    virtual bool isFinished() const=0;
    virtual RaceResult getResult() const=0;
    virtual void setDifficulty(Difficulty d){ m_diff=d; }
    void setFont(const sf::Font& f){ m_font=f; }
protected:
    Difficulty m_diff=Difficulty::MEDIUM;
    sf::Font   m_font;
};
