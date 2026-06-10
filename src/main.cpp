#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include "Car.hpp"
#include "HUD.hpp"
#include "Collectible.hpp"
#include "Garage.hpp"
#include "Map.hpp"

int main() {
    // 1. Tworzymy okno
    sf::RenderWindow window(sf::VideoMode(1280, 720), "NitroDrive - Test Milestone v0.2");
    window.setFramerateLimit(60);

    // 2. Ladowanie glownej czcionki
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        std::cerr << "Nie udalo sie wczytac czcionki!" << std::endl;
    }

    // 3. Generowanie pustych tekstur testowych, zeby kod nie sypal bledami o brak plikow
    sf::Texture carTex, coinTex, nitroTex, bgTex;
    carTex.create(40, 70);
    coinTex.create(30, 30);
    nitroTex.create(30, 40);
    bgTex.create(1280, 720);

    std::array<sf::Texture, 8> skins;
    for (int i = 0; i < 8; i++) skins[i].create(40, 70);

    // 4. Inicjalizacja obiektow gry
    Map myMap;
    myMap.generate(MapType::CIRCUIT);

    Car myCar(carTex);

    // Ustawiamy pozycje startowa na podstawie mapy
    if (!myMap.getStartPositions().empty()) {
        myCar.setPosition(myMap.getStartPositions()[2]);
    } else {
        myCar.setPosition(sf::Vector2f(640.f, 360.f));
    }

    HUD myHUD;
    myHUD.loadFont("C:/Windows/Fonts/arial.ttf");

    Garage myGarage;
    myGarage.init(font, bgTex, skins);
    myGarage.setCoins(2500); // Dajemy sobie troche gotowki na start do testow sklepu

    // Ladujemy wstepne statystyki auta z garazu
    myCar.applyStats(myGarage.buildStats());

    // Ustawiamy znajdzki na mapie
    std::vector<Collectible> items;
    items.push_back(Collectible(CollectibleType::COIN, sf::Vector2f(400.f, 300.f), coinTex, nitroTex));
    items.push_back(Collectible(CollectibleType::NITRO, sf::Vector2f(700.f, 450.f), coinTex, nitroTex));

    // Zmienne pomocnicze
    sf::Clock clock;
    bool inGarage = false; // Flaga przelaczania ekranow
    sf::View camera(sf::FloatRect(0, 0, 1280, 720)); // Kamera podazajaca za graczem

    // 5. Glowna petla gry
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Zmiana ekranu za pomoca przycisku (dla trybu gry)
            if (!inGarage && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::G) {
                inGarage = true;
            }

            // Przekazanie eventow do Garazu
            if (inGarage) {
                myGarage.handleEvent(event, window);
            }
        }

        if (inGarage) {
            // Metoda update w Garage zwraca m_back. Jesli true, znaczy ze kliknelismy BACK.
            if (myGarage.update(dt)) {
                inGarage = false;
                // Aktualizujemy auto nowymi czesciami po zamknieciu sklepu
                myCar.applyStats(myGarage.buildStats());
            }
        } else {
            // Logika i sterowanie autem (Strzalki / Spacja)
            float thr = 0.f, brk = 0.f, str = 0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) thr = 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) brk = 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) str = -1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) str = 1.f;

            // Kara za zjechanie z asfaltu na trawe
            if (!myMap.isOnTrack(myCar.getPosition())) {
                thr *= 0.25f;
            }

            myCar.setThrottle(thr);
            myCar.setBrake(brk);
            myCar.setSteering(str);
            myCar.setHandbrake(sf::Keyboard::isKeyPressed(sf::Keyboard::Space));

            myCar.update(dt);

            // Aktualizacja kamery
            auto p = myCar.getPosition();
            auto sz = myMap.getSize();
            camera.setCenter(std::clamp(p.x, 640.f, std::max(640.f, sz.x - 640.f)),
                             std::clamp(p.y, 360.f, std::max(360.f, sz.y - 360.f)));

            // Kolizje i animacje znajdziek
            for (auto& item : items) {
                item.update(dt);
                if (!item.isCollected() && item.getBounds().intersects(myCar.getBounds())) {
                    item.collect();
                    if (item.getType() == CollectibleType::NITRO) {
                        myCar.addNitro(50.f); // Dodaje nitro do baku
                    }
                    // Docelowo dla COIN dodasz np. gotowke do garazu lub zapiszesz do save'a
                }
            }
        }

        window.clear(sf::Color(30, 35, 45)); // Mroczne tlo "asfaltu"

        if (inGarage) {
            // Przywracamy domyslny widok dla interfejsu
            window.setView(window.getDefaultView());

            // Rysujemy tylko garaz, jezeli jestesmy w srodku
            myGarage.draw(window);
        } else {
            // Kamera przypisana do swiata gry
            window.setView(camera);

            // Rysujemy mape pod spodem
            myMap.draw(window);

            // Rysujemy przedmioty
            for (auto& item : items) {
                item.draw(window);
            }

            // Rysujemy auto
            myCar.draw(window);

            // Przywracamy standardowy widok ekranu (1280x720) zeby narysowac HUD
            window.setView(window.getDefaultView());

            // Pakowanie na zywo informacji z auta do interfejsu
            HUDData hd;
            hd.speed = myCar.getSpeed();
            hd.gear = myCar.getGear();
            hd.nitro = myCar.getNitro();
            hd.nitroMax = myCar.getNitroMax();
            hd.showDrift = myCar.isDrifting();
            hd.driftAngle = myCar.getDriftAngle();
            hd.coins = 2500; // Do testu, docelowo z pobierania salda z Garazu / Save'a
            hd.raceTime = 0.f;

            // Rysowanie HUD-a na samej gorze
            myHUD.draw(window, hd, window.getSize());
        }

        window.display();
    }

    return 0;
}
