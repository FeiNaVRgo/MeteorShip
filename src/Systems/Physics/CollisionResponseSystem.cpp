#include "CollisionResponseSystem.h"

#include "Components/Enemy.h"
#include "Components/Player.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Projectiles/ProjectileSpawner.h"

namespace ms::systems {
    namespace {
        bool sameFaction(ecs::ECS& ecs, const ecs::Entity first, const ecs::Entity second) {
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

        sf::Vector2f projectileHitNormal(
            const sf::FloatRect& projectileBounds,
            const sf::FloatRect& targetBounds) {
            const auto overlap = projectileBounds.findIntersection(targetBounds);
            if (!overlap) {
                return {};
            }

            const float projectileCenterX =
                projectileBounds.position.x + projectileBounds.size.x / 2.f;
            const float targetCenterX =
                targetBounds.position.x + targetBounds.size.x / 2.f;
            const float projectileCenterY =
                projectileBounds.position.y + projectileBounds.size.y / 2.f;
            const float targetCenterY =
                targetBounds.position.y + targetBounds.size.y / 2.f;

            if (overlap->size.x < overlap->size.y) {
                return {projectileCenterX < targetCenterX ? -1.f : 1.f, 0.f};
            }
            return {0.f, projectileCenterY < targetCenterY ? -1.f : 1.f};
        }

        void handleProjectileHit(
            ecs::ECS& ecs,
            const ecs::Entity projectileEntity,
            const ecs::Entity targetEntity,
            const sf::Vector2f hitNormal) {
            auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);
            if (projectile.penetrating && !projectile.piercing &&
                !projectile.hitEntities.insert(targetEntity).second) {
                return;
            }

            const auto modifierHooks = projectile.modifierHooks;
            const bool survivesHit = projectile.piercing || projectile.penetrating;
            const auto triggerType = projectile.triggerType;
            inventory::ModifierContext context{
                .entityManager = ecs,
                .projectileEntity = projectileEntity,
                .targetEntity = targetEntity,
                .hitNormal = hitNormal,
                .deltaTime = 0.f
            };
            const bool hitTrigger = triggerType == inventory::TriggerType::Collision ||
                triggerType == inventory::TriggerType::Timer;
            if (hitTrigger) {
                releaseProjectilePayload(ecs, projectileEntity);
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

    CollisionResponseSystem::CollisionResponseSystem(utility::EventHandling& eventHandler) {
        collisionDetectedConn = utility::EventSignalFactory::createEventSignal<Event::CollisionDetected>(
            eventHandler,
            [this](const Event::CollisionDetected& event) { handleCollisionDetected(event); });
    }

    void CollisionResponseSystem::handleCollisionDetected(const Event::CollisionDetected& event) {
        auto& ecs = getWorld().getECS();
        const auto first = event.first;
        const auto second = event.second;
        if (!ecs.isEntityAlive(first) || !ecs.isEntityAlive(second) ||
            !ecs.hasComponent<components::Rigidbody>(first) ||
            !ecs.hasComponent<components::Rigidbody>(second) ||
            !ecs.hasComponent<components::Transform>(first) ||
            !ecs.hasComponent<components::Transform>(second)) {
            return;
        }

        auto& firstBody = ecs.getComponent<components::Rigidbody>(first);
        auto& secondBody = ecs.getComponent<components::Rigidbody>(second);
        const auto overlap = firstBody.bounds.findIntersection(secondBody.bounds);
        if (!overlap) {
            return;
        }

        const bool firstIsProjectile = ecs.hasComponent<components::Projectile>(first);
        const bool secondIsProjectile = ecs.hasComponent<components::Projectile>(second);
        if (firstIsProjectile || secondIsProjectile) {
            if (firstIsProjectile && secondIsProjectile) {
                return;
            }

            const auto projectileEntity = firstIsProjectile ? first : second;
            const auto targetEntity = firstIsProjectile ? second : first;
            const auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);
            if (!canProjectileHit(ecs, projectile, targetEntity)) {
                return;
            }

            const auto hitNormal = projectileHitNormal(
                ecs.getComponent<components::Rigidbody>(projectileEntity).bounds,
                ecs.getComponent<components::Rigidbody>(targetEntity).bounds);
            handleProjectileHit(ecs, projectileEntity, targetEntity, hitNormal);
            return;
        }

        sf::Vector2f firstCorrection{};
        if (overlap->size.x < overlap->size.y) {
            const float firstCenter = firstBody.bounds.position.x + firstBody.bounds.size.x / 2.f;
            const float secondCenter = secondBody.bounds.position.x + secondBody.bounds.size.x / 2.f;
            const float direction = firstCenter == secondCenter
                                        ? (first < second ? -1.f : 1.f)
                                        : (firstCenter < secondCenter ? -1.f : 1.f);
            firstCorrection.x = direction * overlap->size.x / 2.f;
        } else {
            const float firstCenter = firstBody.bounds.position.y + firstBody.bounds.size.y / 2.f;
            const float secondCenter = secondBody.bounds.position.y + secondBody.bounds.size.y / 2.f;
            const float direction = firstCenter == secondCenter
                                        ? (first < second ? -1.f : 1.f)
                                        : (firstCenter < secondCenter ? -1.f : 1.f);
            firstCorrection.y = direction * overlap->size.y / 2.f;
        }

        const sf::Vector2f secondCorrection = -firstCorrection;
        ecs.getComponent<components::Transform>(first).move(firstCorrection);
        ecs.getComponent<components::Transform>(second).move(secondCorrection);
        firstBody.bounds.position += firstCorrection;
        secondBody.bounds.position += secondCorrection;

        const auto removeNormalVelocity = [](components::Rigidbody& body, sf::Vector2f correction) {
            if (correction.x != 0.f) {
                body.velocity.x = 0.f;
            } else {
                body.velocity.y = 0.f;
            }
        };
        removeNormalVelocity(firstBody, firstCorrection);
        removeNormalVelocity(secondBody, secondCorrection);
    }
}
