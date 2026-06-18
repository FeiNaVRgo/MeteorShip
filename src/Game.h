//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <SFML/Graphics.hpp>
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Events/EventHandling.h"

namespace ms {
    class Game {
        ecs::ECS                ecs;
        sf::RenderWindow        mainWindow;
        systems::Quadtree       m_quadtree;
        utility::EventHandling  eventHandler;
        EventDispatcher         eventDispatcher;
    public:
        Game() :
            mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default),
            m_quadtree({{0.f, 0.f}, {static_cast<float>(mainWindow.getSize().x), static_cast<float>(mainWindow.getSize().y)}}),
            eventHandler(*this) {
            mainWindow.setFramerateLimit(60);

            //eventHandler.bindToSignal<sf::Event::Closed>([]() {std::cout << "closing window" << std::endl;}); //for testing

            ecs.registerComponent<components::Rigidbody>();
            ecs.registerComponent<components::Transform>();

            ecs.registerSystem<systems::SpatialHash>(m_quadtree);
        }

        auto getEcs() -> ecs::ECS& {
            return ecs;
        }

        auto getMainWindow() -> sf::RenderWindow& {
            return mainWindow;
        }

        auto getQuadtree() -> systems::Quadtree& {
            return m_quadtree;
        }

        void draw() {
            //how do I even handle drawing things qwq
            //should I just have a queue of function that draw
            //or have a general component called drawable?
            //and based on it have a system dat just iterates
            //over every entity with drawable component and then
            //handle every entity properly
            //but then how do I even model drawable class
            //everything will be drawn on mainWindow probably qwq
            //hmmm there is something called sf::VertexArrays
            m_quadtree.draw(mainWindow);
        }

        void update(sf::Time const& elapsed) {
            ecs.update(elapsed.asSeconds());
        }

        void handleEvents() {
            while (const auto event = mainWindow.pollEvent()) {
                eventHandler.handleEvent(event);
            }

            while (const auto event = eventDispatcher.poll()) {
                eventHandler.handleEvent(event);
            }
        }

        void run() {
            sf::Clock clock;

            while (mainWindow.isOpen()) {
                sf::Time elapsed = clock.restart();

                handleEvents();
                update(elapsed);

                mainWindow.clear(sf::Color::Black);
                draw();
                mainWindow.display();
            }
        }
    };
} // ms

#endif //METEORSHIP_GAME_H
