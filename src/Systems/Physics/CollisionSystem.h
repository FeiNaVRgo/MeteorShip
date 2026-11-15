#ifndef METEORSHIP_COLLISIONSYSTEM_H
#define METEORSHIP_COLLISIONSYSTEM_H

#include <utility>
#include <vector>

#include "ECS/ECS.h"

namespace ms::systems {
    class CollisionResponseSystem;

    class CollisionSystem : public ecs::System {
        CollisionResponseSystem* collisionResponseSystem = nullptr;

    public:
        explicit CollisionSystem(CollisionResponseSystem& responseSystem)
            : collisionResponseSystem(&responseSystem) {}
        explicit CollisionSystem()
            : collisionResponseSystem(nullptr) {}

        void update(float deltaTime) override;

        void setCollisionResponseSystem(CollisionResponseSystem* pcollisionResponseSystem);
    };
}

#endif //METEORSHIP_COLLISIONSYSTEM_H

