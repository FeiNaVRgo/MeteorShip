#ifndef METEORSHIP_HITSYSTEM_H
#define METEORSHIP_HITSYSTEM_H

#include "ECS/ECS.h"

namespace ms::systems {
    class HitSystem : public ecs::System {
    public:
        void update(float deltaTime) override;
    };
}

#endif //METEORSHIP_HITSYSTEM_H
