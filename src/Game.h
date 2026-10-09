//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <SFML/Graphics.hpp>
#include "Components/Drawable.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Events/EventHandling.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Drawing/DrawingSystem.h"
#include "Utility/AssetManager.h"

namespace ms {
    class Game {
        //should we have some global values?
        //like ecs and render window hmm
        ecs::ECS                ecs;
        sf::RenderWindow        mainWindow;
        systems::Quadtree       m_quadtree;
        utility::EventHandling  eventHandler;
        EventDispatcher         eventDispatcher;
        utility::AssetManager   assetManager;
    public:
        Game() :
            mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default),
            m_quadtree({{0.f, 0.f}, {static_cast<float>(mainWindow.getSize().x), static_cast<float>(mainWindow.getSize().y)}}),
            eventHandler(*this) {
            mainWindow.setFramerateLimit(60);

            assetManager.loadAssets();

            //eventHandler.bindToSignal<sf::Event::Closed>([]() {std::cout << "closing window" << std::endl;}); //for testing

            //TODO: try to make a player entity with view on this entity :3
            //TODO: loading textures
            //TODO: loading shaders
            //think about how to initialize them
            //maybe have a class that is something like AssetHandler/Loader
            ecs.registerComponent<components::Rigidbody>();
            ecs.registerComponent<components::Transform>();
            ecs.registerComponent<components::Drawable>();

            ecs.registerSystem<systems::SpatialHash>(m_quadtree);
            ecs.registerSystem<systems::DrawingSystem>(mainWindow);

            utility::EventSignalFactory::createEventSignal<sf::Event::KeyReleased>(eventHandler, [](sf::Event::KeyReleased const& event) {
                if (event.code == sf::Keyboard::Key::A) {
                    std::cout << "A" << std::endl;
                }
            });
            createPlayer();
        }

        void createPlayer() {
            auto player = ecs.createEntity();

            components::Transform transform{};
            transform.setRotation(sf::degrees(0));
            transform.setPosition(sf::Vector2f(mainWindow.getSize().x / 2.f, mainWindow.getSize().y / 2.f));
            ecs.addComponent<components::Transform>(player, transform);

            components::Drawable drawable{};
            /*
            sf::VertexArray triangle(sf::PrimitiveType::Triangles, 3);

            triangle[0].position = sf::Vector2f(10.f, 10.f);
            triangle[1].position = sf::Vector2f(100.f, 10.f);
            triangle[2].position = sf::Vector2f(100.f, 100.f);

            triangle[0].color = sf::Color::Red;
            triangle[1].color = sf::Color::Blue;
            triangle[2].color = sf::Color::Green;

            drawable.vertices = triangle;
            */
            drawable.sprite = sf::Sprite(assetManager.textureMap.at("Bullets-spritesheet"));
            ecs.addComponent<components::Drawable>(player, drawable);
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

                mainWindow.clear(sf::Color::Black);
                draw();
                update(elapsed);
                mainWindow.display();
            }
        }
    };
} // ms

#endif //METEORSHIP_GAME_H
