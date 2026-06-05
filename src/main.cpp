#include <SFML/Graphics.hpp>
#include "Car.hpp"

int main() {
    //czy sfml dziala?
    sf::RenderWindow window(sf::VideoMode(800, 600), "NitroDrive - Test Milestone v0.1");

    // tworzymy auto
    Car myCar;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);

        // w przyszlosci rysujemy auto

        window.display();
    }
    return 0;
}