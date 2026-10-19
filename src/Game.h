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

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SFML_GL3_IMPLEMENTATION

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

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
