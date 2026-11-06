#ifndef METEORSHIP_COLLISIONSYSTEM_H
#define METEORSHIP_COLLISIONSYSTEM_H

#include <memory>
#include <utility>
#include <vector>

#include "ECS/ECS.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"

namespace ms::systems {
    class CollisionSystem : public ecs::System {
        class OpenCLBackend;

        Quadtree &quadtree;
        EventDispatcher &eventDispatcher;
        std::unique_ptr<OpenCLBackend> gpuBackend;
        std::vector<std::pair<ecs::Entity, ecs::Entity> > collisions;
        bool gpuFailureReported{};

    public:
        CollisionSystem(Quadtree &quadtree, EventDispatcher &eventDispatcher);

        ~CollisionSystem() override;

        void update(float deltaTime) override;

        [[nodiscard]] bool isUsingGpu() const { return gpuBackend != nullptr; }

        [[nodiscard]] const std::vector<std::pair<ecs::Entity, ecs::Entity> > &
        getCollisions() const { return collisions; }
    };
}

#endif //METEORSHIP_COLLISIONSYSTEM_H
