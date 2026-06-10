#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include <array>
#include "Car.hpp"
#include "Bot.hpp"
#include "HUD.hpp"
#include "Collectible.hpp"
#include "Garage.hpp"
#include "Map.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "NitroDrive - Bot Test");
    window.setFramerateLimit(60);

    // --- Czcionka ---
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf"))
        std::cerr << "Brak czcionki!\n";


    //  TEKSTURY AUT  (assets/cars/car_formula_0..9.png)
    std::array<sf::Texture, 10> carTextures;
    for (int i = 0; i < 10; i++) {
        std::string path = "assets/cars/car_formula_" + std::to_string(i) + ".png";
        if (!carTextures[i].loadFromFile(path)) {
            carTextures[i].create(40, 70);
            std::cerr << "Brak tekstury: " << path << " — uzyto fallback\n";
        }
        carTextures[i].setSmooth(true);
    }

    // Skins do garazu (pierwsze 8 tekstur)
    std::array<sf::Texture, 8> skins;
    for (int i = 0; i < 8; i++) skins[i] = carTextures[i];


    sf::Texture coinTex, nitroTex, bgTex;
    { sf::Image img; img.create(30, 30, sf::Color{255, 215, 0});  coinTex.loadFromImage(img); }
    { sf::Image img; img.create(30, 30, sf::Color{0, 180, 255}); nitroTex.loadFromImage(img); }
    bgTex.create(1280, 720);

    //  MAPA
    Map myMap;
    myMap.generate(MapType::CIRCUIT);
    const auto& wps    = myMap.getWaypoints();
    const auto& starts = myMap.getStartPositions();

    //  GARAZ

    Garage myGarage;
    myGarage.init(font, bgTex, skins);
    myGarage.setCoins(2500);
    CarStats baseStats = myGarage.buildStats();
    int playerCoins = 2500;

    //  GRACZ  — tekstura 0

    Car myCar(carTextures[0]);
    myCar.setPosition(starts.size() > 0 ? starts[0] : sf::Vector2f(640.f, 360.f));
    if (wps.size() >= 2) {
        sf::Vector2f d = wps[1] - wps[0];
        myCar.setAngle(std::atan2(d.y, d.x) * 180.f / 3.14159f);
    }
    myCar.applyStats(baseStats);

    // BOTY
    struct BotCfg { int texIdx; Difficulty diff; const char* name; };
    BotCfg cfgs[4] = {
                      { 1, Difficulty::HARD,   "BOT1 HARD" },
                      { 2, Difficulty::MEDIUM, "BOT2 MED"  },
                      { 3, Difficulty::MEDIUM, "BOT3 MED"  },
                      { 4, Difficulty::EASY,   "BOT4 EASY" },
                      };

    std::vector<Bot> bots;
    bots.reserve(4);
    for (int i = 0; i < 4; i++) {
        bots.emplace_back(carTextures[cfgs[i].texIdx], sf::Color::White);
        bots[i].setBotId(i);
        bots[i].setWaypoints(wps);
        bots[i].setDifficulty(cfgs[i].diff);
        bots[i].setMaxLaps(3);
        bots[i].applyStats(baseStats);
        sf::Vector2f pos = (starts.size() > (size_t)(i + 1))
                               ? starts[i + 1]
                               : sf::Vector2f(640.f + i * 70.f, 360.f);
        bots[i].setPosition(pos);
        if (wps.size() >= 2) {
            sf::Vector2f d = wps[1] - wps[0];
            bots[i].setAngle(std::atan2(d.y, d.x) * 180.f / 3.14159f);
        }
    }

    //  COLLECTIBLES
    //  Pozycje dobrane recznie na mapie CIRCUIT
    std::vector<Collectible> items;
    // Monety — 4 sztuki
    items.emplace_back(CollectibleType::COIN,  sf::Vector2f(2050.f, 600.f), coinTex, nitroTex);
    items.emplace_back(CollectibleType::COIN,  sf::Vector2f(1200.f, 182.f), coinTex, nitroTex);
    items.emplace_back(CollectibleType::COIN,  sf::Vector2f( 350.f, 600.f), coinTex, nitroTex);
    items.emplace_back(CollectibleType::COIN,  sf::Vector2f(1200.f,1015.f), coinTex, nitroTex);
    // Nitro — 2 sztuki na prostych
    items.emplace_back(CollectibleType::NITRO, sf::Vector2f(1600.f, 220.f), coinTex, nitroTex);
    items.emplace_back(CollectibleType::NITRO, sf::Vector2f( 800.f, 980.f), coinTex, nitroTex);


    //  HUD

    HUD myHUD;
    myHUD.loadFont("C:/Windows/Fonts/arial.ttf");

    sf::Clock clock;
    float raceTime = 0.f;
    bool inGarage  = false;

    // Cooldown zmiany biegow — jeden event = jedna zmiana
    float shiftUpCD   = 0.f;
    float shiftDownCD = 0.f;
    const float SHIFT_CD = 0.25f;

    sf::View camera(sf::FloatRect(0, 0, 1280, 720));

    // Etykieta nad botami
    sf::Text botLabel;
    botLabel.setFont(font);
    botLabel.setCharacterSize(13);
    botLabel.setOutlineColor(sf::Color::Black);
    botLabel.setOutlineThickness(1.5f);

    // Popup "zebrano" (+coins / +nitro)
    struct Popup { std::string txt; sf::Vector2f pos; float ttl; sf::Color col; };
    std::vector<Popup> popups;

    // MAIN petla
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        shiftUpCD   -= dt;
        shiftDownCD -= dt;

        // --- Popupy ---
        for (auto& p : popups) { p.ttl -= dt; p.pos.y -= 40.f * dt; }
        popups.erase(std::remove_if(popups.begin(), popups.end(),
                                    [](const Popup& p){ return p.ttl <= 0.f; }), popups.end());

        // --- Eventy ---
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                // G — garaz
                if (event.key.code == sf::Keyboard::G)
                    inGarage = !inGarage;

                if (!inGarage) {
                    // Shift — bieg w gore
                    if (event.key.code == sf::Keyboard::LShift && shiftUpCD <= 0.f) {
                        myCar.shiftUp();
                        shiftUpCD = SHIFT_CD;
                    }
                    // LCtrl — bieg w dol
                    if (event.key.code == sf::Keyboard::LControl && shiftDownCD <= 0.f) {
                        myCar.shiftDown();
                        shiftDownCD = SHIFT_CD;
                    }
                    // R — restart
                    if (event.key.code == sf::Keyboard::R) {
                        myCar.reset();
                        myCar.setPosition(starts.size() > 0 ? starts[0] : sf::Vector2f(640.f, 360.f));
                        if (wps.size() >= 2) {
                            sf::Vector2f d = wps[1] - wps[0];
                            myCar.setAngle(std::atan2(d.y, d.x) * 180.f / 3.14159f);
                        }
                        for (int i = 0; i < 4; i++) {
                            bots[i].reset();
                            bots[i].resetProgress();
                            sf::Vector2f pos = (starts.size() > (size_t)(i + 1))
                                                   ? starts[i + 1]
                                                   : sf::Vector2f(640.f + i * 70.f, 360.f);
                            bots[i].setPosition(pos);
                            if (wps.size() >= 2) {
                                sf::Vector2f d = wps[1] - wps[0];
                                bots[i].setAngle(std::atan2(d.y, d.x) * 180.f / 3.14159f);
                            }
                        }
                        raceTime = 0.f;
                        popups.clear();
                    }
                }
            }

            if (inGarage)
                myGarage.handleEvent(event, window);
        }

        //  UPDATE
        if (inGarage) {
            if (myGarage.update(dt)) {
                inGarage = false;
                // Synchronizuj monety z garazu
                SaveData tmpSD;
                myGarage.syncToSave(tmpSD);
                playerCoins = tmpSD.coins;
                CarStats s = myGarage.buildStats();
                myCar.applyStats(s);
                for (auto& b : bots) b.applyStats(s);
            }
        } else {
            raceTime += dt;

            // --- Sterowanie WSAD ---
            float thr = 0.f, brk = 0.f, str = 0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) thr =  1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) brk =  1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) str = -1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) str =  1.f;

            // E — nitro (trzymasz = aktywne)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
                myCar.activateNitro();

            // Kara za trawe
            if (!myMap.isOnTrack(myCar.getPosition())) thr *= 0.25f;

            myCar.setThrottle(thr);
            myCar.setBrake(brk);
            myCar.setSteering(str);
            myCar.setHandbrake(sf::Keyboard::isKeyPressed(sf::Keyboard::Space));
            myCar.update(dt);

            // --- Boty ---
            for (auto& b : bots)
                b.updateAI(dt);

            // --- Kolizje gracz <-> boty
            for (auto& b : bots) {
                if (myCar.getBounds().intersects(b.getBounds())) {
                    myCar.applyCollisionImpact(b.getSpeed());
                    b.applyCollisionImpact(myCar.getSpeed());
                }
            }

            // --- Collectibles: update + kolizja z graczem
            for (auto& item : items) {
                item.update(dt);

                if (!item.isCollected() && item.getBounds().intersects(myCar.getBounds())) {
                    item.collect();
                    sf::Vector2f pp = myCar.getPosition();

                    if (item.getType() == CollectibleType::NITRO) {
                        myCar.addNitro(40.f);
                        popups.push_back({ "+NITRO", pp, 1.2f, {0, 200, 255, 255} });
                    } else {
                        playerCoins += 50;
                        myGarage.setCoins(playerCoins);
                        popups.push_back({ "+$50", pp, 1.2f, {255, 215, 0, 255} });
                    }
                }
            }

            // --- Kamera
            auto p  = myCar.getPosition();
            auto sz = myMap.getSize();
            camera.setCenter(
                std::clamp(p.x, 640.f, std::max(640.f, sz.x - 640.f)),
                std::clamp(p.y, 360.f, std::max(360.f, sz.y - 360.f)));
        }

        //draw
        window.clear(sf::Color(25, 30, 40));

        if (inGarage) {
            window.setView(window.getDefaultView());
            myGarage.draw(window);
        } else {
            // --- Swiat ---
            window.setView(camera);
            myMap.draw(window);

            // Collectibles
            for (auto& item : items)
                item.draw(window);

            // Boty
            for (auto& b : bots) b.draw(window);

            // Gracz
            myCar.draw(window);

            // Etykiety botow
            for (int i = 0; i < 4; i++) {
                botLabel.setFillColor(sf::Color::White);
                botLabel.setString(cfgs[i].name);
                auto bp = bots[i].getPosition();
                botLabel.setPosition(bp.x - 22.f, bp.y - 38.f);
                window.draw(botLabel);
            }

            // Popupy (+coins, +nitro) — rysowane
            sf::Text popTxt;
            popTxt.setFont(font);
            popTxt.setCharacterSize(18);
            popTxt.setOutlineColor(sf::Color::Black);
            popTxt.setOutlineThickness(1.5f);
            for (auto& p : popups) {
                popTxt.setString(p.txt);
                popTxt.setFillColor(sf::Color(p.col.r, p.col.g, p.col.b,
                                              (uint8_t)std::clamp(p.ttl / 1.2f * 255.f, 0.f, 255.f)));
                popTxt.setPosition(p.pos);
                window.draw(popTxt);
            }

            //  HUD
            window.setView(window.getDefaultView());
            HUDData hd;
            hd.speed      = myCar.getSpeed();
            hd.gear       = myCar.getGear();
            hd.nitro      = myCar.getNitro();
            hd.nitroMax   = myCar.getNitroMax();
            hd.showDrift  = myCar.isDrifting();
            hd.driftAngle = myCar.getDriftAngle();
            hd.raceTime   = raceTime;
            hd.coins      = playerCoins;
            hd.showLap    = false;
            hd.offTrack   = !myMap.isOnTrack(myCar.getPosition());
            myHUD.draw(window, hd, window.getSize());

            // Legenda na dole
            sf::Text info;
            info.setFont(font);
            info.setCharacterSize(13);
            info.setFillColor({200, 200, 200, 180});
            info.setOutlineColor(sf::Color::Black);
            info.setOutlineThickness(1.f);
            info.setString(
                "WSAD: jedz  |  E: nitro  |  LShift: bieg+  |  LCtrl: bieg-"
                "  |  Spacja: reczny  |  R: restart  |  G: garaz");
            info.setPosition(10.f, (float)window.getSize().y - 20.f);
            window.draw(info);
        }

        window.display();
    }

    return 0;
}