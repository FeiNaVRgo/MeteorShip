#ifndef METEORSHIP_COLLISIONSYSTEM_H
#define METEORSHIP_COLLISIONSYSTEM_H

#include <utility>
#include <vector>

#include "ECS/ECS.h"

namespace ms::systems {
    class CollisionSystem : public ecs::System {
        std::vector<std::pair<ecs::Entity, ecs::Entity> > collisions;

    public:
        void update(float deltaTime) override;

        [[nodiscard]] const std::vector<std::pair<ecs::Entity, ecs::Entity> > &
        getCollisions() const { return collisions; }
    };
}

#endif //METEORSHIP_COLLISIONSYSTEM_H
