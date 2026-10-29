#ifndef METEORSHIP_PROJECTILE_H
#define METEORSHIP_PROJECTILE_H

#include <optional>
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
        float lifeTime = 0.f;
        float timerLifeTime = 0.f;
        bool friendlyFire = false;
        bool piercing = false;
        bool penetrating = false;
        bool homing = false;
        float homingTurnRate = 0.f;
        std::optional<float> maxTravelDistance;
    };
}

#endif //METEORSHIP_PROJECTILE_H
