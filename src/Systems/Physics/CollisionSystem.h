#ifndef METEORSHIP_COLLISIONSYSTEM_H
#define METEORSHIP_COLLISIONSYSTEM_H

#include <memory>
#include <utility>
#include <vector>

#include "ECS/ECS.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"

namespace ms::systems {
    class OpenCLCollisionDetector;

    class CollisionSystem : public ecs::System {
        Quadtree &quadtree;
        EventDispatcher &eventDispatcher;
        OpenCLCollisionDetector &collisionDetector;
        std::vector<std::pair<ecs::Entity, ecs::Entity> > collisions;

    public:
        CollisionSystem(Quadtree &quadtree, EventDispatcher &eventDispatcher,
                        OpenCLCollisionDetector &collisionDetector);

        void update(float deltaTime) override;

        [[nodiscard]] const std::vector<std::pair<ecs::Entity, ecs::Entity> > &
        getCollisions() const { return collisions; }
    };
}

#endif //METEORSHIP_COLLISIONSYSTEM_H
