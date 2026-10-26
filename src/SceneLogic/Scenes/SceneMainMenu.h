//
// Created by iformat on 10/25/26.
//

#ifndef METEORSHIP_SCENEMAINMENU_H
#define METEORSHIP_SCENEMAINMENU_H
#include "SceneBase.h"

namespace ms::scene_logic::scenes {
    class SceneMainMenu : public SceneBase {
    public:
        explicit SceneMainMenu(Game &game, const std::string &name);

        void init() override;
        void destroy() override;
        void update(float elapsedTime) override;
    };
}

#endif //METEORSHIP_SCENEMAINMENU_H
