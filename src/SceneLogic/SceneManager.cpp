//
// Created by iformat on 10/15/26.
//

#include "SceneManager.h"

#include <stdexcept>

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
        if (!scene) throw std::invalid_argument("Cannot register a null scene.");
        const auto name = scene->getName();
        const auto [iterator, inserted] = scenes.try_emplace(name, std::move(scene));
        if (!inserted) {
            throw std::logic_error("A scene named '" + name + "' is already registered.");
        }
        worlds.push_back(iterator->second->getWorld());
    }

}
