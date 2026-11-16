//
// Created by iformat on 10/21/26.
//

#ifndef METEORSHIP_BEHAVIORSYSTEM_H
#define METEORSHIP_BEHAVIORSYSTEM_H
#include "BTNodes/BTContext.h"
#include "Components/BehaviorTree.h"
#include "ECS/ECS.h"
#include "ECS/World.h"

namespace ms::systems {
    class BehaviorSystem : public ecs::System {
    public:
        void update(float deltaTime) override {
            getWorld().getECS().view<components::BehaviorTree>().each(
                [deltaTime](ecs::Entity const&, components::BehaviorTree& behaviorTree) {
                    auto btContext = behaviorTree.blackboard->get<bt_nodes::BTContext>("bt_context");
                    btContext.deltaTime = deltaTime;
                    behaviorTree.blackboard->set("bt_context", btContext);
                    behaviorTree.BTTree.tickOnce();
            });
        }
    };
}

#endif //METEORSHIP_BEHAVIORSYSTEM_H
