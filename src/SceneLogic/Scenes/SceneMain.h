//
// Created by iformat on 10/15/26.
//

#ifndef METEORSHIP_SCENEMAIN_H
#define METEORSHIP_SCENEMAIN_H

#include <boost/signals2/connection.hpp>

#include "Inventory/Inventory.h"
#include "SceneBase.h"
#include "Utility/CustomEntityLoader.h"

namespace ms::scene_logic::scenes {
    class SceneMain : public SceneBase {
        inventory::Inventory inventory{
            inventory::DefaultSpellSlotCount,
            inventory::DefaultTurretMountCount
        };
        std::vector<utility::CustomEnemyDefinition> enemyDefinitions;
        float simulationAccumulator = 0.f;

    public:
        boost::signals2::connection removeEntityConn;

        explicit SceneMain(Game& game, std::string name);

        void init() override;
        void destroy() override;
        void update(float elapsedTime) override;

        void createPlayer();

        void createEnemy();
        void createEnemy(const utility::CustomEnemyDefinition& definition);

    };
}

#endif //METEORSHIP_SCENEMAIN_H
