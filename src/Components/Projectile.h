#ifndef METEORSHIP_PROJECTILE_H
#define METEORSHIP_PROJECTILE_H

#include "ECS/ECS.h"

namespace ms::components {
    struct Projectile {
        ecs::Entity owner;
        float damage = 1.f;
    };
}

#endif //METEORSHIP_PROJECTILE_H
