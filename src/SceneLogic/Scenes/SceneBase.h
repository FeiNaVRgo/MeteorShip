//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEBASE_H
#define METEORSHIP_SCENEBASE_H

#include <memory>
#include <string>

#include "ECS/World.h"
#include "UI/UI.h"

namespace ms {
    class Game;
}

struct nk_context;

namespace ms::scene_logic::scenes {
    class SceneBase {
    private:
        std::string name;
    protected:
        Game& game;
        std::shared_ptr<World> world;
        ui::UI ui;
    public:
        SceneBase(Game &game, std::string name);
        virtual ~SceneBase() = default;

        std::string const& getName() { return name; }
        std::shared_ptr<World> getWorld() { return world; }
        void drawUI(nk_context* ctx);

        virtual void init() = 0;
        virtual void destroy() = 0;
        virtual void update(float elapsedTime) = 0;
    };
}

#endif //METEORSHIP_SCENEBASE_H
