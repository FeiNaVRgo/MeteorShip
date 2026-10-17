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
#include "Events/EventHandling.h"
#include "Systems/Drawing/DrawingSystem.h"
#include "Utility/AssetManager.h"
#include "SceneLogic/SceneManager.h"
#include "SceneLogic/Scenes/SceneMain.h"

namespace ms {
    class Game {
        WorldBox                    worldBox;
        sf::RenderWindow            mainWindow;
        utility::EventHandling      eventHandler;
        utility::AssetManager       assetManager;
        scene_logic::SceneManager   sceneManager;
    public:
        Game() : worldBox(),
                 mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default),
                 eventHandler(*this),
                 sceneManager(worldBox.getWorlds()) {
            mainWindow.setFramerateLimit(60);

            assetManager.loadAssets();

            //scenes need to be added before registration of components
            sceneManager.addScene(std::make_unique<scene_logic::scenes::SceneMain>(*this, "main"));

            worldBox.registerComponent<components::Rigidbody>();
            worldBox.registerComponent<components::Transform>();
            worldBox.registerComponent<components::Drawable>();
            worldBox.registerComponent<components::Player>();
            worldBox.registerComponent<components::ShootingAccess>();

            sceneManager.changeScene("main");
        }


        auto getMainWindow() -> sf::RenderWindow& {
            return mainWindow;
        }

        utility::AssetManager& getAssetManager() { return assetManager; }
        utility::EventHandling& getEventHandler() { return eventHandler; }

        void update(sf::Time const& elapsed) {
            sceneManager.updateCurrentScene(elapsed.asSeconds());
        }

        void handleEvents() {
            while (auto const event = mainWindow.pollEvent()) {
                eventHandler.handleEvent(event);
            }

            while (auto const event = sceneManager.getCurrentScene()->getWorld()->getEventDispatcher().poll()) {
                eventHandler.handleEvent(event);
            }
        }

        void run() {
            sf::Clock clock;

            while (mainWindow.isOpen()) {
                sf::Time elapsed = clock.restart();

                handleEvents();

                mainWindow.clear(sf::Color::Black);

                update(elapsed);

                mainWindow.display();
            }
        }
    };
} // ms

#endif //METEORSHIP_GAME_H
