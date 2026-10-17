//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEBASE_H
#define METEORSHIP_SCENEBASE_H

#include <memory>
#include <string>

#include "ECS/World.h"

namespace ms {
    class Game;
}

namespace ms::scene_logic::scenes {
    class SceneBase {
    private:
        //ui
        std::string name;
    protected:
        Game& game;
        std::shared_ptr<World> world;
    public:
        SceneBase(Game &game, std::string name);
        //virtual ~SceneBase() = default;

        std::string const& getName() { return name; }
        std::shared_ptr<World> const& getWorld() { return world; }
        virtual void init() = 0;
        virtual void destroy() = 0;
        virtual void update(float elapsedTime) = 0;
    };
}

#endif //METEORSHIP_SCENEBASE_H
