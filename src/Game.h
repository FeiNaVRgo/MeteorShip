//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <SFML/Graphics.hpp>
#include "Components/Drawable.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/Player.h"
#include "Components/ShootingAccess.h"
#include "ECS/ECS.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Events/EventHandling.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Drawing/DrawingSystem.h"
#include "Systems/Player/PlayerSystem.h"
#include "Utility/AssetManager.h"

namespace ms {
    class Game {
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

            ecs.registerComponent<components::Rigidbody>();
            ecs.registerComponent<components::Transform>();
            ecs.registerComponent<components::Drawable>();
            ecs.registerComponent<components::Player>();
            ecs.registerComponent<components::ShootingAccess>();

            ecs.registerSystem<systems::SpatialHash>(m_quadtree);
            ecs.registerSystem<systems::DrawingSystem>(mainWindow);
            ecs.registerSystem<systems::PlayerSystem>(eventDispatcher);

            utility::EventSignalFactory::createEventSignal<Event::PlayerShot>(eventHandler, [this](Event::PlayerShot const& event) {
                createPlayerBullet();
            });

            createPlayer();
        }

        void createPlayerBullet() {
            auto const bullet = ecs.createEntity();


        }

        void createPlayer() {
            auto const player = ecs.createEntity();

            ecs.addComponent<components::Player>(player);
            ecs.addComponent<components::ShootingAccess>(player, components::ShootingAccess(1.f));

            components::Transform transform{};
            transform.setRotation(sf::degrees(0));
            transform.setPosition(sf::Vector2f(mainWindow.getSize().x / 2.f, mainWindow.getSize().y / 2.f));
            ecs.addComponent<components::Transform>(player, transform);

            components::Drawable drawable{};
            drawable.sprite = assetManager.spriteMap.at("JetFighter-Var3-Spritesheet");
            ecs.addComponent<components::Drawable>(player, drawable);

            components::Rigidbody rigidbody{};
            auto rect = drawable.sprite.value().sprite.getLocalBounds();
            rect.position = transform.getPosition();
            rigidbody.bounds = rect;
            ecs.addComponent<components::Rigidbody>(player, rigidbody);

            m_quadtree.insert(player, rigidbody.bounds);
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
            m_quadtree.draw(mainWindow);
        }

        void update(sf::Time const& elapsed) {
            ecs.update(elapsed.asSeconds());
        }

        void handleEvents() {
            while (auto const event = mainWindow.pollEvent()) {
                eventHandler.handleEvent(event);
            }

            while (auto const event = eventDispatcher.poll()) {
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
