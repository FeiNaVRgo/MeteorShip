//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEBASE_H
#define METEORSHIP_SCENEBASE_H

#include <string>

namespace ms {
    class Game;
}

namespace ms::scene_logic::scenes {
    class SceneBase {
    private:
        //ui
        Game& game;
        std::string name;
    public:
        SceneBase(Game &game, std::string name);
        //virtual ~SceneBase() = default;

        std::string const& getName() { return name; }
        virtual void init() = 0;
        virtual void destroy() = 0;
        virtual void update() = 0;
    };
}

#endif //METEORSHIP_SCENEBASE_H
