#pragma once
#include "GameMode.hpp"
#include "Map.hpp"
#include "HUD.hpp"
#include "ParticleSystem.hpp"

class CircuitMode : public GameMode {
public:
    CircuitMode();
    void init(Car* p, std::vector<std::unique_ptr<Bot>>& b) override;
    void handleInput(const sf::Event& e) override;
    void update(float dt) override;
    void draw(sf::RenderTarget& t) override;
    bool isFinished() const override { return m_done; }
    RaceResult getResult() const override { return m_result; }
private:
    Car* m_player=nullptr;
    std::vector<std::unique_ptr<Bot>>* m_bots=nullptr;
    Map m_map; HUD m_hud; ParticleSystem m_psys;
    sf::View m_camera;
    HUDData m_hd; RaceResult m_result;
    sf::Texture m_coinTex, m_nitroTex; bool m_texOk=false;
    std::vector<Collectible> m_cols;
    float m_raceTime=0, m_lapTime=0, m_bestLap=0;
    int   m_lap=1, m_coins=0, m_wpIdx=0;
    bool  m_done=false;
    bool  m_crossedFinish=false;
    float m_finishCooldown=0.f;
    void buildTex();
    int  calcPos() const;
};
