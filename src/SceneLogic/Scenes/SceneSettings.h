//
// Created by iformat on 10/26/26.
//

#ifndef METEORSHIP_SCENESETTINGS_H
#define METEORSHIP_SCENESETTINGS_H

#include "SceneBase.h"

namespace ms::scene_logic::scenes {
    class SceneSettings : public SceneBase {
    public:
        explicit SceneSettings(Game &game, const std::string &name);

        void init() override;
        void destroy() override;
        void update(float elapsedTime) override;
    };
}

#endif //METEORSHIP_SCENESETTINGS_H
