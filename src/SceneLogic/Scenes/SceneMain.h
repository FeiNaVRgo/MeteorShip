//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEMAIN_H
#define METEORSHIP_SCENEMAIN_H

#include "SceneBase.h"

namespace ms::scene_logic::scenes {
    class SceneMain : public SceneBase {
    public:
        explicit SceneMain(Game& game)
            : SceneBase(game, "main") {
        }

        void init() override;
        void destroy() override;
        void update() override;
    };
}

#endif //METEORSHIP_SCENEMAIN_H
