#include "HitResponseSystem.h"

#include "Components/Enemy.h"
#include "Components/Player.h"
#include "Components/Projectile.h"
#include "ECS/World.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Projectiles/ProjectileSpawner.h"

namespace ms::systems {
    namespace {
        bool sameFaction(
            ecs::ECS& ecs,
            const ecs::Entity first,
            const ecs::Entity second) {
            return (ecs.hasComponent<components::Player>(first) &&
                    ecs.hasComponent<components::Player>(second)) ||
                   (ecs.hasComponent<components::Enemy>(first) &&
                    ecs.hasComponent<components::Enemy>(second));
        }

        bool canProjectileHit(
            ecs::ECS& ecs,
            const components::Projectile& projectile,
            const ecs::Entity targetEntity) {
            const bool ownerAlive = ecs.isEntityAlive(projectile.owner);
            if (ownerAlive && targetEntity == projectile.owner) {
                return false;
            }
            if (projectile.friendlyFire || !ownerAlive) {
                return true;
            }
            return !sameFaction(ecs, projectile.owner, targetEntity);
        }

        void handleProjectileHit(
            World& world,
            const ecs::Entity projectileEntity,
            const ecs::Entity targetEntity,
            const sf::Vector2f hitNormal) {
            auto& ecs = world.getECS();
            auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);
            if (projectile.penetrating && !projectile.piercing &&
                !projectile.hitEntities.insert(targetEntity).second) {
                return;
            }

            const auto modifierHooks = projectile.modifierHooks;
            const bool survivesHit = projectile.piercing || projectile.penetrating;
            const auto triggerType = projectile.triggerType;
            inventory::ModifierContext context{
                .world = world,
                .projectileEntity = projectileEntity,
                .targetEntity = targetEntity,
                .hitNormal = hitNormal,
                .deltaTime = 0.f
            };
            if (triggerType == inventory::TriggerType::Collision ||
                triggerType == inventory::TriggerType::Timer) {
                releaseProjectilePayload(world, projectileEntity);
            }

            for (const auto& hook : modifierHooks) {
                if (hook.onCollision) {
                    hook.onCollision(context);
                }
            }
            if (survivesHit || !ecs.isEntityAlive(projectileEntity) ||
                !ecs.hasComponent<components::Projectile>(projectileEntity)) {
                return;
            }

            for (const auto& hook : modifierHooks) {
                if (hook.onDeath) {
                    hook.onDeath(context);
                }
            }
            if (ecs.isEntityAlive(projectileEntity)) {
                ecs.destroyEntity(projectileEntity);
            }
        }
    }

    HitResponseSystem::HitResponseSystem(utility::EventHandling& eventHandler) {
        hitDetectedConnection =
            utility::EventSignalFactory::createEventSignal<Event::HitDetected>(
                eventHandler,
                [this](const Event::HitDetected& event) { handleHitDetected(event); });
    }

    void HitResponseSystem::handleHitDetected(const Event::HitDetected& event) {
        auto& ecs = getWorld().getECS();
        const auto first = event.first;
        const auto second = event.second;
        if (!ecs.isEntityAlive(first) || !ecs.isEntityAlive(second)) {
            return;
        }

        const bool firstIsProjectile = ecs.hasComponent<components::Projectile>(first);
        const bool secondIsProjectile = ecs.hasComponent<components::Projectile>(second);
        if (firstIsProjectile == secondIsProjectile) {
            return;
        }

        const auto projectileEntity = firstIsProjectile ? first : second;
        const auto targetEntity = firstIsProjectile ? second : first;
        const auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);
        if (!canProjectileHit(ecs, projectile, targetEntity)) {
            return;
        }

        handleProjectileHit(
            getWorld(),
            projectileEntity,
            targetEntity,
            {});
    }
}
