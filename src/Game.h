//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <iostream>
#include <SFML/Graphics.hpp>
#include "ECS/ECS.h"

namespace ms {
    class Game {
    public:
        static auto run() -> void {
            // create the window
            sf::RenderWindow window(sf::VideoMode({800, 600}), "My window");

            // run the program as long as the window is open
            while (window.isOpen()) {
                // check all the window's events that were triggered since the last iteration of the loop
                while (const auto event = window.pollEvent()) {
                    // "close requested" event: we close the window
                    if (event->is<sf::Event::Closed>()) {
                        window.close();
                    }
                    else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                        if (key->scancode == sf::Keyboard::Scancode::Escape) {
                            window.close();
                        }
                    }
                }

                // clear the window with black color
                window.clear(sf::Color::Magenta);

                // draw everything here...
                // window.draw(...);

                // end the current frame
                window.display();
            }
        }
    };
} // ms

#endif //METEORSHIP_GAME_H
