//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <SFML/Graphics.hpp>

#include "Components/Rigidbody.h"
#include "ECS/ECS.h"
#include "Systems/SpatialHashing/Quadtree.h"

namespace ms {
    class Game {
        ecs::ECS ecs;
        sf::RenderWindow mainWindow;
        systems::Quadtree m_quadtree;
    public:
        Game() :
            mainWindow(sf::VideoMode({800, 600}), "My window"),
            m_quadtree({{0.f, 0.f}, {static_cast<float>(mainWindow.getSize().x), static_cast<float>(mainWindow.getSize().y)}}) {}
        void run() {
            ecs.registerComponent<ms::components::Rigidbody>();
            for (int i = 0; i < 9; i ++) {
                auto e = ecs.createEntity();
                ecs.addComponent<ms::components::Rigidbody>(e, sf::Vector2f{0.f, 0.f}, sf::Vector2f{0.f, 0.f}, sf::FloatRect{{1.f, 1.f}, {10.f, 10.f}});
                m_quadtree.insert(e, ecs.getComponent<ms::components::Rigidbody>(e).bounds);
            }

            // run the program as long as the window is open
            while (mainWindow.isOpen()) {
                // check all the window's events that were triggered since the last iteration of the loop
                while (const auto event = mainWindow.pollEvent()) {
                    // "close requested" event: we close the window
                    if (event->is<sf::Event::Closed>()) {
                        mainWindow.close();
                    }
                    else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                        if (key->scancode == sf::Keyboard::Scancode::Escape) {
                            mainWindow.close();
                        }
                        if (key->scancode == sf::Keyboard::Scancode::R) {
                            m_quadtree.remove(3);
                        }
                    }
                }

                // clear the window with black color
                mainWindow.clear(sf::Color::Black);

                m_quadtree.draw(mainWindow);


                // end the current frame
                mainWindow.display();
            }
        }
    };
} // ms

#endif //METEORSHIP_GAME_H
