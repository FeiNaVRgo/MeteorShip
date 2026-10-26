//
// Created by iformat on 10/15/26.
//

#include "SceneManager.h"

namespace ms::scene_logic {
    SceneManager::SceneManager(std::vector<std::shared_ptr<World>>& worlds)
    : currentScene(nullptr),
      worlds(worlds) {}

    SceneManager::~SceneManager() = default;

    void SceneManager::updateCurrentScene(float elapsedTime) const {
        if (currentScene != nullptr) currentScene->update(elapsedTime);
    }

    void SceneManager::changeScene(std::string const& name) {
        if (currentScene != nullptr) currentScene->destroy();
        currentScene = scenes.at(name).get();
        currentScene->init();
    }


    void SceneManager::addScene(std::unique_ptr<scenes::SceneBase> scene) {
        worlds.push_back(scene->getWorld());
        scenes.try_emplace(scene->getName(), std::move(scene));
    }

}
