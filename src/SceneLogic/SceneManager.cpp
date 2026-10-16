//
// Created by iformat on 10/15/26.
//

#include "SceneManager.h"

namespace ms::scene_logic {
    SceneManager::SceneManager() : currentScene(nullptr) {
    }
    SceneManager::~SceneManager() = default;

    void SceneManager::updateCurrentScene() const {
        if (currentScene != nullptr) currentScene->update();
    }

    void SceneManager::changeScene(std::string const& name) {
        if (scenes.contains(name)) {
            if (currentScene != nullptr) currentScene->destroy();

            auto scene = scenes.at(name);
            scene->update();
            currentScene = scene;
        }
    }

    void SceneManager::addScene(scenes::SceneBase* scene) {
        scenes[scene->getName()] = scene;
    }
}
