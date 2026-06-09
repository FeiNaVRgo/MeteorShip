//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <SFML/Graphics.hpp>
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "Systems/SpatialHashing/Quadtree.h"
#include "Systems/SpatialHashing/SpatialHash.h"

namespace ms {
    class Game {

        ecs::ECS ecs;
        sf::RenderWindow mainWindow;
        systems::Quadtree m_quadtree;
    public:
        Game() :
            mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default),
            m_quadtree({{0.f, 0.f}, {static_cast<float>(mainWindow.getSize().x), static_cast<float>(mainWindow.getSize().y)}}) {

            mainWindow.setFramerateLimit(60);

            ecs.registerComponent<components::Rigidbody>();
            ecs.registerComponent<components::Transform>();

            ecs.registerSystem<systems::SpatialHash>(m_quadtree);

        }

        void draw() {
            m_quadtree.draw(mainWindow);
        }

        void update(sf::Time const& elapsed) {
            ecs.update(elapsed.asSeconds());
        }

        void run() {
            sf::Clock clock;

            while (mainWindow.isOpen()) {
                sf::Time elapsed = clock.restart();
                while (const auto event = mainWindow.pollEvent()) {
                    if (event->is<sf::Event::Closed>()) {
                        mainWindow.close();
                    }
                }

                update(elapsed);

                mainWindow.clear(sf::Color::Black);
                draw();
                mainWindow.display();
            }
        }
    };
} // ms

#endif //METEORSHIP_GAME_H
