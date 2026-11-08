#ifndef METEORSHIP_PROJECTILESYSTEM_H
#define METEORSHIP_PROJECTILESYSTEM_H

#include <vector>

#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "ECS/World.h"
#include "Systems/Projectiles/ProjectileSpawner.h"

namespace ms::systems {
    class ProjectileSystem : public ecs::System {
    public:
        void update(const float deltaTime) override {
            struct ExpiredProjectile {
                ecs::Entity entity;
                std::vector<inventory::ProjectileModifierHook> hooks;
                bool releasePayload;
            };

            auto& world = getWorld();
            auto& entityManager = world.getECS();
            std::vector<ExpiredProjectile> expiredProjectiles;
            entityManager.view<components::Projectile>().each(
                [this, deltaTime, &expiredProjectiles](const ecs::Entity entity,
                                                       components::Projectile& projectile) {
                    if (projectile.pendingRemoval) {
                        return;
                    }

                    const auto hooks = projectile.modifierHooks;
                    inventory::ModifierContext context{
                        .entityManager = getWorld().getECS(),
                        .projectileEntity = entity,
                        .targetEntity = 0,
                        .hitNormal = {},
                        .deltaTime = deltaTime
                    };

                    for (const auto& hook : hooks) {
                        if (hook.onUpdate) hook.onUpdate(context);
                    }

                    projectile.age += deltaTime;
                    projectile.lifeTime -= deltaTime;
                    bool timerElapsed = false;
                    if (projectile.triggerType == inventory::TriggerType::Timer &&
                        !projectile.triggered) {
                        projectile.timer -= deltaTime;
                        timerElapsed = projectile.timer <= 0.f;
                    }

                    if (timerElapsed || projectile.lifeTime <= 0.f) {
                        projectile.pendingRemoval = true;
                        const bool releasePayload = timerElapsed ||
                            projectile.triggerType == inventory::TriggerType::Expiration;
                        expiredProjectiles.push_back({entity, hooks, releasePayload});
                    }
                });

            for (const auto& expired : expiredProjectiles) {
                if (!entityManager.isEntityAlive(expired.entity)) {
                    continue;
                }
                if (expired.releasePayload) {
                    releaseProjectilePayload(entityManager, expired.entity);
                }

                inventory::ModifierContext context{
                    .entityManager = entityManager,
                    .projectileEntity = expired.entity,
                    .targetEntity = 0,
                    .hitNormal = {},
                    .deltaTime = deltaTime
                };
                for (const auto& hook : expired.hooks) {
                    if (hook.onDeath) hook.onDeath(context);
                }
                if (entityManager.isEntityAlive(expired.entity)) {
                    world.getEventDispatcher().push(Event::RemoveEntity(expired.entity));
                }
            }
        }
    };
}

#endif //METEORSHIP_PROJECTILESYSTEM_H