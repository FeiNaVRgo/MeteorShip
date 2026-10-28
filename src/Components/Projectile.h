#ifndef METEORSHIP_PROJECTILE_H
#define METEORSHIP_PROJECTILE_H

#include <vector>

#include "Components/Damage.h"
#include "ECS/ECS.h"

namespace ms::components {
    struct Projectile {
        ecs::Entity owner;
        float damage = 1.f;
        std::vector<DamageAmount> damageTypes{{DamageType::Projectile, 1.f}};
        float radius = 0.f;
        float critChanceBonus = 0.f;
    };
}

#endif //METEORSHIP_PROJECTILE_H
