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
        std::unordered_map<std::string, scenes::SceneBase*> scenes;
    public:
        SceneManager();
        ~SceneManager();

        void updateCurrentScene() const;
        void changeScene(std::string const& name);
        void addScene(scenes::SceneBase* scene);
    };
}

#endif //METEORSHIP_SCENEMANAGER_H
