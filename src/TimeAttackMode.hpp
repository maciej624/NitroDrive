#pragma once
#include "GameMode.hpp"
#include "Map.hpp"
#include "HUD.hpp"
#include "ParticleSystem.hpp"

// TIME ATTACK: 3 okrazenia solo, bez botow, liczy sie najlepszy lap i poprawa rekordu
class TimeAttackMode : public GameMode {
public:
    TimeAttackMode();
    void init(Car* p, std::vector<std::unique_ptr<Bot>>& b) override;// Inicjalizuje stan wyscigu (ustawia gracza, ignoruje wektor botow)
    void handleInput(const sf::Event& e) override;// Przechwytuje akcje uzytkownika
    void update(float dt) override;// Glowna petla logiczna: aktualizuje fizyke, czas, kolizje i parametry gry
    void draw(sf::RenderTarget& t) override;// Renderuje mape, pojazd, efekty czasteczkowe i interfejs (HUD)
    bool isFinished() const override { return m_done; }// Informuje, czy wyscig dobiegl konca (przejechano wymagana liczbe okrazen)
    RaceResult getResult() const override { return m_result; }// Zwraca zsumowane statystyki i wyniki po zakonczeniu jazdy
    void setPreviousBest(float pb){ m_prevBest=pb; }// Pozwala wgrac najlepszy czas z poprzednich sesji, aby porownywac z nim wyniki
private:
    Car* m_player=nullptr;
    Map m_map; HUD m_hud; ParticleSystem m_psys;
    sf::View m_camera;
    HUDData m_hd; RaceResult m_result;
    sf::Texture m_coinTex, m_nitroTex; bool m_texOk=false;
    std::vector<Collectible> m_cols;
    float m_raceTime=0, m_lapTime=0, m_bestLap=0;
    float m_prevBest=0.f;       // poprzedni rekord (do porownania)
    float m_ghostSector=0.f;    // czas sektoru ghost (uproszczony)
    int   m_lap=1, m_coins=0, m_wpIdx=0;
    bool  m_done=false;
    bool  m_crossedFinish=false;
    float m_finishCooldown=0.f;
    // Sektor-timer (co 1/3 okrazenia pokazuje delta vs. best lap)
    float m_sectorProgress=0.f;
    float m_deltaTime=0.f;       // ronica do best lapa
    bool  m_showDelta=false;
    float m_showDeltaTimer=0.f;
    void buildTex();
};
