#ifndef METEORSHIP_PROJECTILE_H
#define METEORSHIP_PROJECTILE_H

#include <vector>
#include "ECS/ECS.h"
#include "Inventory/Spell.h"

namespace ms::components {
    struct Projectile {
        ecs::Entity owner{};
        float lifeTime = 1.f;
        float age = 0.f;
        bool friendlyFire = false;
        bool piercing = false;
        inventory::TriggerType triggerType = inventory::TriggerType::None;
        float timer = 0.f;
        std::vector<inventory::ShootableSpell> payload;
        std::vector<inventory::ProjectileModifierHook> modifierHooks;
    };
}

#endif //METEORSHIP_PROJECTILE_H
