//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEBASE_H
#define METEORSHIP_SCENEBASE_H

#include <utility>
#include <string>
#include "../../Game.h"

namespace ms::scene_logic::scenes {
    class SceneBase {
    private:
        //ui
        Game& game;
        std::string name;
    public:
        explicit SceneBase(Game& game, std::string name) : game(game), name(std::move(name)) {}
        virtual ~SceneBase() = default;

        std::string const& getName() { return name; }
        virtual void init();
        virtual void destroy();
        virtual void update();
    };
}

#endif //METEORSHIP_SCENEBASE_H
