//
// Created by iformat on 10/21/26.
//

#ifndef METEORSHIP_BEHAVIORSYSTEM_H
#define METEORSHIP_BEHAVIORSYSTEM_H
#include "Components/BehaviorTree.h"
#include "ECS/ECS.h"
#include "ECS/World.h"

namespace ms::systems {
    class BehaviorSystem : public ecs::System {
    public:
        void update(float deltaTime) override {
            getWorld().getECS().view<components::BehaviorTree>().each(
                [](ecs::Entity const& entity, components::BehaviorTree& behaviorTree) {
                    behaviorTree.BTTree.tickOnce();
            });
        }
    };
}

#endif //METEORSHIP_BEHAVIORSYSTEM_H
