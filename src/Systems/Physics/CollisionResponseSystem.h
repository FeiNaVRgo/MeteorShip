#ifndef METEORSHIP_COLLISIONRESPONSESYSTEM_H
#define METEORSHIP_COLLISIONRESPONSESYSTEM_H

#include "ECS/ECS.h"
#include "Events/Event.h"

namespace ms::systems {
    class CollisionResponseSystem : public ecs::System {
    public:
        [[nodiscard]] bool resolveCollision(const Event::CollisionDetected& event);
    };
}

#endif //METEORSHIP_COLLISIONRESPONSESYSTEM_H
