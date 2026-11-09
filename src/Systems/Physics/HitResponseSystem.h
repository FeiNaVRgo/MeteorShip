#ifndef METEORSHIP_HITRESPONSESYSTEM_H
#define METEORSHIP_HITRESPONSESYSTEM_H

#include <boost/signals2/connection.hpp>

#include "ECS/ECS.h"
#include "Events/Event.h"

namespace ms::utility {
    class EventHandling;
}

namespace ms::systems {
    class HitResponseSystem : public ecs::System {
        boost::signals2::scoped_connection hitDetectedConnection;

    public:
        explicit HitResponseSystem(utility::EventHandling& eventHandler);

    private:
        void handleHitDetected(const Event::HitDetected& event);
    };
}

#endif //METEORSHIP_HITRESPONSESYSTEM_H
