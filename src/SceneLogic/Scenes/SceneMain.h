//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEMAIN_H
#define METEORSHIP_SCENEMAIN_H

#include <boost/signals2/connection.hpp>
#include "SceneBase.h"

namespace ms::scene_logic::scenes {
    class SceneMain : public SceneBase {
    public:
        boost::signals2::connection playerShotConn;
        boost::signals2::connection removeEntityConn;

        explicit SceneMain(Game& game, std::string name);

        void init() override;
        void destroy() override;
        void update(float elapsedTime) override;

        void createPlayerBullet(ecs::Entity player);

        void createPlayer();
    };
}

#endif //METEORSHIP_SCENEMAIN_H
