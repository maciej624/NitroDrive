#pragma once
#include "GameMode.hpp"
#include "Map.hpp"
#include "HUD.hpp"
#include "ParticleSystem.hpp"

class DragMode : public GameMode {
public:
    DragMode();
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
    float m_raceTime=0, m_finishX=3040;
    int   m_coins=0;
    bool  m_done=false;
    void buildTex();
};
