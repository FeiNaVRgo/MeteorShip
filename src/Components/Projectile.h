#ifndef METEORSHIP_PROJECTILE_H
#define METEORSHIP_PROJECTILE_H

#include <unordered_set>
#include <vector>
#include "ECS/ECS.h"
#include "Inventory/Spell.h"

namespace ms::components {
    struct Projectile {
        ecs::Entity owner{};
        float lifeTime = 1.f; //seconds
        float age = 0.f;
        float speedMultiplier = 1.f;
        bool friendlyFire = false;
        bool piercing = false;
        bool triggered = false;
        bool pendingRemoval = false;
        inventory::TriggerType triggerType = inventory::TriggerType::None;
        inventory::SpellFormation payloadFormation;
        float timer = 0.f;
        std::vector<inventory::ShootableSpell> payload;
        std::vector<inventory::ProjectileModifierHook> modifierHooks;
        std::unordered_set<ecs::Entity> hitEntities;
    };
}

#endif //METEORSHIP_PROJECTILE_H
