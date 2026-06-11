#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include <array>
#include <memory>
#include "Car.hpp"
#include "Bot.hpp"
#include "HUD.hpp"
#include "Collectible.hpp"
#include "Garage.hpp"
#include "Map.hpp"
#include "ParticleSystem.hpp"
#include "CircuitMode.hpp"
#include "DragMode.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "NitroDrive - Bot Test");
    window.setFramerateLimit(60);

    // wczytanie glownej czcionki
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf"))
        std::cerr << "Brak czcionki!\n";

    // wczytanie tekstur samochodow z zabezpieczeniem w razie bledu
    std::array<sf::Texture, 10> carTextures;
    for (int i = 0; i < 10; i++) {
        std::string path = "assets/cars/car_formula_" + std::to_string(i) + ".png";
        if (!carTextures[i].loadFromFile(path)) {
            carTextures[i].create(40, 70);
            std::cerr << "Brak tekstury: " << path << " — uzyto fallback\n";
        }
        carTextures[i].setSmooth(true);
    }

    // pierwsze 8 aut idzie jako skiny do garazu
    std::array<sf::Texture, 8> skins;
    for (int i = 0; i < 8; i++) skins[i] = carTextures[i];

    sf::Texture bgTex;
    bgTex.create(1280, 720);

    // inicjalizacja garazu i wczytanie startowych statystyk
    Garage myGarage;
    myGarage.init(font, bgTex, skins);
    myGarage.setCoins(2500);
    CarStats baseStats = myGarage.buildStats();
    int playerCoins = 2500;

    // ustawienie auta gracza na starcie (wybrany tryb zajmie sie ustawieniem pozycji na mapie)
    Car myCar(carTextures[0]);
    myCar.applyStats(baseStats);

    // konfiguracja poziomow trudnosci i wygladu botow
    struct BotCfg { int texIdx; Difficulty diff; const char* name; };
    BotCfg cfgs[4] = {
                      { 1, Difficulty::HARD,   "BOT1 HARD" },
                      { 2, Difficulty::MEDIUM, "BOT2 MED"  },
                      { 3, Difficulty::MEDIUM, "BOT3 MED"  },
                      { 4, Difficulty::EASY,   "BOT4 EASY" },
                      };

    // respawn botow z uzyciem inteligentnych wskaznikow
    std::vector<std::unique_ptr<Bot>> bots;
    bots.reserve(4);
    for (int i = 0; i < 4; i++) {
        bots.push_back(std::make_unique<Bot>(carTextures[cfgs[i].texIdx], sf::Color::White));
        bots[i]->setBotId(i);
        bots[i]->setDifficulty(cfgs[i].diff);
        bots[i]->applyStats(baseStats);
        // przypisanie trasy i pozycji startowej wykona teraz metoda init danego trybu
    }

    // inicjalizacja trybu wyscigu (okrążenia)
    CircuitMode circuit;
    circuit.setFont(font);

    // inicjalizacja trybu drag
    DragMode drag;

    // domyślnie startujemy z trybem okrążeń
    bool isDragMode = false;
    circuit.init(&myCar, bots);

    sf::Clock clock;
    bool inGarage = false;
    bool raceFinishedHandled = false;

    // glowna petla gry
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        // obsluga wejscia
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                // przelaczanie widoku garazu
                if (event.key.code == sf::Keyboard::G) {
                    inGarage = !inGarage;
                    if (inGarage) {
                        myGarage.setCoins(playerCoins); // aktualizacja stanu konta w garazu
                    }
                }

                if (!inGarage) {
                    // zmiana trybu gry klawiszem M (Circuit <-> Drag)
                    if (event.key.code == sf::Keyboard::M) {
                        isDragMode = !isDragMode;
                        raceFinishedHandled = false;
                        if (isDragMode) {
                            drag.init(&myCar, bots);
                        } else {
                            circuit.init(&myCar, bots);
                        }
                    }
                    // reset wyscigu i odtworzenie aut na starcie aktualnego toru
                    else if (event.key.code == sf::Keyboard::R) {
                        raceFinishedHandled = false;
                        if (isDragMode) {
                            drag.init(&myCar, bots);
                        } else {
                            circuit.init(&myCar, bots);
                        }
                    } else {
                        // przekazanie reszty wejscia (biegi, nitro) do odpowiedniego trybu
                        if (isDragMode) {
                            drag.handleInput(event);
                        } else {
                            circuit.handleInput(event);
                        }
                    }
                }
            }

            if (inGarage)
                myGarage.handleEvent(event, window);
        }

        // logika gry
        if (inGarage) {
            if (myGarage.update(dt)) {
                inGarage = false;
                // powrot z garazu i zaktualizowanie kupionych statystyk
                SaveData tmpSD;
                myGarage.syncToSave(tmpSD);
                playerCoins = tmpSD.coins;
                CarStats s = myGarage.buildStats();
                myCar.applyStats(s);
                for (auto& b : bots) b->applyStats(s);
            }
        } else {
            // cala logika fizyki, botow, collectibli, HUD i kamery zaktualizowana przez aktywny tryb
            if (isDragMode) {
                drag.update(dt);
                // sprawdzanie czy wyscig sie skonczyl zeby dodac zebrane w trakcie wyscigu monety (tylko raz)
                if (drag.isFinished() && !raceFinishedHandled) {
                    playerCoins += drag.getResult().coinsEarned;
                    raceFinishedHandled = true;
                }
            } else {
                circuit.update(dt);
                // sprawdzanie czy wyscig sie skonczyl zeby dodac zebrane w trakcie wyscigu monety (tylko raz)
                if (circuit.isFinished() && !raceFinishedHandled) {
                    playerCoins += circuit.getResult().coinsEarned;
                    raceFinishedHandled = true;
                }
            }
        }

        // renderowanie klatki
        window.clear(sf::Color(25, 30, 40));

        if (inGarage) {
            window.setView(window.getDefaultView());
            myGarage.draw(window);
        } else {
            // cale rysowanie swiata, pojazdow, efektow czasteczkowych i HUD delegowane do aktywnej klasy
            if (isDragMode) {
                drag.draw(window);
            } else {
                circuit.draw(window);
            }

            // podpowiedz ze sterowaniem na dole ekranu (nakladana absolutnie na interfejs UI)
            sf::Text info;
            info.setFont(font);
            info.setCharacterSize(13);
            info.setFillColor({200, 200, 200, 180});
            info.setOutlineColor(sf::Color::Black);
            info.setOutlineThickness(1.f);
            // zaktualizowany string z informacja o zmianie trybu i zlamaniem linii dla czytelnosci
            info.setString(
                "WSAD: jedz  |  E: nitro  |  LShift: bieg+  |  LCtrl: bieg-\n"
                "Spacja: reczny  |  R: restart  |  G: garaz  |  M: zmien tryb (Circuit/Drag)");
            // lekko podniesione w osi Y by zmiescic dwie linijki
            info.setPosition(10.f, (float)window.getSize().y - 38.f);
            window.draw(info);
        }

        window.display();
    }

    return 0;
}
