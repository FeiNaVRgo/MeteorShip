#ifndef METEORSHIP_COLLISIONRESPONSESYSTEM_H
#define METEORSHIP_COLLISIONRESPONSESYSTEM_H

#include <boost/signals2/connection.hpp>

#include "ECS/ECS.h"
#include "Events/Event.h"

namespace ms::utility {
    class EventHandling;
}

namespace ms::systems {
    class CollisionResponseSystem : public ecs::System {
        boost::signals2::scoped_connection collisionDetectedConn;

    public:
        explicit CollisionResponseSystem(utility::EventHandling& eventHandler);

    private:
        void handleCollisionDetected(const Event::CollisionDetected& event);
    };
}

#endif //METEORSHIP_COLLISIONRESPONSESYSTEM_H
