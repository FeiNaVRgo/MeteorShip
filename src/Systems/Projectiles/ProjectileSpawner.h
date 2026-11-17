#ifndef METEORSHIP_PROJECTILESPAWNER_H
#define METEORSHIP_PROJECTILESPAWNER_H

#include "ECS/ECS.h"
#include "Inventory/Spell.h"

namespace ms {
    class World;
}

namespace ms::systems {
    ecs::Entity spawnProjectile(
        World& world,
        ecs::Entity owner,
        sf::Vector2f origin,
        float angleRadians,
        float speedMultiplier,
        const inventory::ShootableSpell& spell);

    void releaseProjectilePayload(World& world, ecs::Entity projectileEntity);
}

#endif //METEORSHIP_PROJECTILESPAWNER_H
