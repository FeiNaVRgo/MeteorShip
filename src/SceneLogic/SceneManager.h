//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEMANAGER_H
#define METEORSHIP_SCENEMANAGER_H

#include "Scenes/SceneBase.h"
#include <unordered_map>
#include <string>

namespace ms::scene_logic {
    class SceneManager {
        scenes::SceneBase* currentScene;
        //string or string_view?
        std::unordered_map<std::string, std::unique_ptr<scenes::SceneBase>> scenes;
        std::vector<std::shared_ptr<World>>& worlds;
    public:
        SceneManager(std::vector<std::shared_ptr<World>>& worlds);
        ~SceneManager();

        void updateCurrentScene(float elapsedTime) const;
        void changeScene(std::string const& name);
        void addScene(std::unique_ptr<scenes::SceneBase> scene);
        [[nodiscard]] scenes::SceneBase* getCurrentScene() const {return currentScene;}
    };
}

#endif //METEORSHIP_SCENEMANAGER_H
