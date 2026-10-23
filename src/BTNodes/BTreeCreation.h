//
// Created by iformat on 10/21/26.
//

#ifndef METEORSHIP_BTREECREATION_H
#define METEORSHIP_BTREECREATION_H

#include "Components/BehaviorTree.h"
#include "ECS/ECS.h"

namespace ms {
    class Game;
}

namespace ms {
    components::BehaviorTree create_tree_for_basic_enemy(Game& game, ecs::Entity entity);
}

#endif //METEORSHIP_BTREECREATION_H
