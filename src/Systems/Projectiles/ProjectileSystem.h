#ifndef METEORSHIP_PROJECTILESYSTEM_H
#define METEORSHIP_PROJECTILESYSTEM_H

#include "Components/Projectile.h"
#include "ECS/ECS.h"

namespace ms::systems {
    class ProjectileSystem : public ecs::System {
    public:
        void update(const float deltaTime) override {
            ecs->view<components::Projectile>().each(
                [this, deltaTime](const ecs::Entity entity, components::Projectile& projectile) {
                    inventory::ModifierContext context{
                        .entityManager = *ecs,
                        .projectileEntity = entity,
                        .targetEntity = 0,
                        .hitNormal = {},
                        .deltaTime = deltaTime
                    };

                    for (const auto& hook : projectile.modifierHooks) {
                        if (hook.onUpdate) hook.onUpdate(context);
                    }
                });
        }
    };
}

#endif //METEORSHIP_PROJECTILESYSTEM_H
