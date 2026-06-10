#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Car.hpp"
#include "SaveSystem.hpp"

class Garage {
public:
    Garage();
    bool init(const sf::Font& f, const sf::Texture& bg,
              const std::array<sf::Texture,8>& skins);
    void syncFromSave(const SaveData& sd);
    void syncToSave(SaveData& sd) const;
    CarStats buildStats() const;
    void setCoins(int c){ m_coins=c; }
    bool update(float dt);
    void handleEvent(const sf::Event& e, sf::RenderWindow& w);
    void draw(sf::RenderTarget& t);

    // koszt ulepszenia
    static constexpr int ENG_C[5]={0,800,1800,3500,6000};
    static constexpr int TIR_C[5]={0,600,1400,2800,5000};
    static constexpr int NIT_C[5]={0,500,1200,2400,4200};
    static constexpr int GBX_C[3]={0,1000,2500};

private:
    sf::Font   m_font;
    sf::Texture m_bgTex;
    sf::Sprite  m_bgSpr;
    std::array<sf::Texture,8> m_skins;
    int m_eng=0, m_tire=0, m_nit=0, m_gear=0, m_skin=0;
    int m_coins=0, m_tab=0;
    sf::Vector2f m_mouse;
    bool m_click=false, m_back=false;

    sf::Text T(const std::string& s, unsigned sz,
               sf::Color c=sf::Color::White) const;
    bool Btn(sf::RenderTarget& t, float x, float y, float w, float h,
             const std::string& lbl, bool bought, bool afford);
    void drawStats(sf::RenderTarget& t, float x, float y);
};
