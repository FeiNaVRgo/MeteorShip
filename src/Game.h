//
// Created by iformat on 6/2/26.
//

#ifndef METEORSHIP_GAME_H
#define METEORSHIP_GAME_H

#include <memory>
#include <string>
#include <unordered_set>

#include <behaviortree_cpp/blackboard.h>
#include <behaviortree_cpp/bt_factory.h>
#include <SFML/Graphics.hpp>
#include <sol/state.hpp>

#include "Events/EventHandling.h"
#include "Utility/AssetManager.h"
#include "SceneLogic/SceneManager.h"
#include "SceneLogic/Scenes/SceneMain.h"
#include "Systems/Physics/OpenCLCollisionDetector.h"
#include "Utility/SaveState/Settings/Settings.h"
#include "Utility/SaveState/SaveState.h"

struct nk_context;
struct nk_font_atlas;

namespace ms {
    class Game {
        WorldBox worldBox;
        sf::ContextSettings sfml_ctx_settings;
        sf::RenderWindow mainWindow;
        sol::state lua;
        utility::EventHandling eventHandler;
        utility::AssetManager assetManager;
        scene_logic::SceneManager sceneManager;
        systems::OpenCLCollisionDetector collisionDetector;
        nk_context *ctx;
        nk_font_atlas *atlas;
        BT::Blackboard::Ptr globalBlackboard;
        BT::BehaviorTreeFactory factory;
        std::unordered_set<std::string> loadedLuaScripts;
        bool closeRequested = false;
        utility::savestate::SaveState st;
        utility::Settings settings;

    public:
        Game();

        sf::RenderWindow &getMainWindow();
        [[nodiscard]] std::shared_ptr<World> getCurrentWorld() const {
            auto* currentScene = sceneManager.getCurrentScene();
            return currentScene ? currentScene->getWorld() : nullptr;
        }
        void requestClose() { closeRequested = true; }

        utility::AssetManager &getAssetManager() { return assetManager; }
        utility::EventHandling &getEventHandler() { return eventHandler; }
        BT::Blackboard::Ptr getGlobalBlackboard() { return globalBlackboard; }
        scene_logic::SceneManager &getSceneManager() { return sceneManager; }
        systems::OpenCLCollisionDetector &getCollisionDetector() { return collisionDetector; }
        BT::BehaviorTreeFactory &getBehaviorTreeFactory() { return factory; }
        sol::state& getLuaState() { return lua; }
        void loadLuaScript(const std::string& path);

        void update(sf::Time const &elapsed) const;

        void handleEvents();

        void run();
    };
} // ms

#endif //METEORSHIP_GAME_H
