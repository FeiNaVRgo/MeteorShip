//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <SFML/Graphics.hpp>
#include "Events/EventHandling.h"
#include "Utility/AssetManager.h"
#include "SceneLogic/SceneManager.h"
#include "SceneLogic/Scenes/SceneMain.h"

struct nk_context;
struct nk_font_atlas;

namespace ms {
    class Game {
        WorldBox                    worldBox;
        sf::ContextSettings         settings;
        sf::RenderWindow            mainWindow;
        utility::EventHandling      eventHandler;
        utility::AssetManager       assetManager;
        scene_logic::SceneManager   sceneManager;
        nk_context*                 ctx;
        nk_font_atlas*              atlas;
    public:
        Game();

        sf::RenderWindow& getMainWindow();

        utility::AssetManager& getAssetManager() { return assetManager; }
        utility::EventHandling& getEventHandler() { return eventHandler; }

        void update(sf::Time const& elapsed) const;

        void handleEvents();

        void run();
    };
} // ms

#endif //METEORSHIP_GAME_H
