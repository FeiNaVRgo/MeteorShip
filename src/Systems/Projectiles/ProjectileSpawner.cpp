#include "ProjectileSpawner.h"

#include <cmath>

#include "Components/Damage.h"
#include "Components/BoundingBox.h"
#include "Components/Drawable.h"
#include "Components/Hitbox.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Utility/UtiFuncs.h"

namespace ms::systems {
    ecs::Entity spawnProjectile(
        World& world,
        const ecs::Entity owner,
        const sf::Vector2f origin,
        const float angleRadians,
        const float speedMultiplier,
        const inventory::ShootableSpell& spellInfo) {
        auto& ecs = world.getECS();
        const auto entity = ecs.createEntity();
        auto& transform = ecs.addComponent<components::Transform>(entity);
        transform.setPosition(origin);
        const auto direction = utility::directionFromAngle(sf::radians(angleRadians));
        transform.setRotation(utility::rotationFromDirection(direction));

        float speed = 500.f;
        if (spellInfo.finalStats.speed) {
            speed = *spellInfo.finalStats.speed * speedMultiplier;
        }

        auto& rigidbody = ecs.addComponent<components::Rigidbody>(entity);
        rigidbody.velocity = direction * speed;

        sf::Vector2f boundsSize(8.f, 8.f);
        auto& projectile = ecs.addComponent<components::Projectile>(entity);
        projectile.owner = owner;
        projectile.speedMultiplier = speedMultiplier;
        projectile.lifeTime = spellInfo.finalStats.lifeTime;
        projectile.friendlyFire = spellInfo.finalStats.friendlyFire;
        projectile.piercing = spellInfo.finalStats.Piercing;
        projectile.penetrating = spellInfo.finalStats.Penetrating;
        projectile.triggerType = spellInfo.finalStats.triggerType;
        projectile.payloadFormation = spellInfo.payloadFormation;
        projectile.timer = spellInfo.finalStats.timerLifeTime.value_or(0.f);
        projectile.payload = spellInfo.payload;
        projectile.modifierHooks = spellInfo.modifierHooks;

        if (const auto* animatedSprite = spellInfo.spell.getProjectileSprite()) {
            components::Drawable drawable;
            drawable.sprite = animatedSprite->createInstance();
            boundsSize = {
                static_cast<float>(animatedSprite->rect.size.x),
                static_cast<float>(animatedSprite->rect.size.y)
            };
            ecs.addComponent<components::Drawable>(entity, std::move(drawable));
        } else if (const auto* icon = spellInfo.spell.getIcon()) {
            components::Drawable drawable;
            drawable.sprite.emplace(icon->getTexture(), icon->getTextureRect(), 1, std::nullopt);
            boundsSize = {
                static_cast<float>(icon->getTextureRect().size.x),
                static_cast<float>(icon->getTextureRect().size.y)
            };
            ecs.addComponent<components::Drawable>(entity, std::move(drawable));
        }

        const auto halfSize = boundsSize / 2.f;
        ecs.addComponent<components::BoundingBox>(
            entity, components::BoundingBox{utility::centeredRect(origin, boundsSize)});
        ecs.addComponent<components::Hitbox>(
            entity,
            components::Hitbox{{sf::FloatRect(-halfSize, boundsSize)}});

        auto& damage = ecs.addComponent<components::DamageAmount>(entity);
        damage.type = components::DamageType::Projectile;
        if (spellInfo.finalStats.damage) {
            damage.amount = *spellInfo.finalStats.damage;
        }

        inventory::ModifierContext context{
            .world = world,
            .projectileEntity = entity,
            .targetEntity = 0,
            .hitNormal = {},
            .deltaTime = 0.f
        };
        const auto modifierHooks = projectile.modifierHooks;
        for (const auto& hook : modifierHooks) {
            if (hook.onInit) {
                hook.onInit(context);
            }
        }

        return entity;
    }

    void releaseProjectilePayload(World& world, const ecs::Entity projectileEntity) {
        auto& ecs = world.getECS();
        if (!ecs.isEntityAlive(projectileEntity) ||
            !ecs.hasComponent<components::Projectile>(projectileEntity) ||
            !ecs.hasComponent<components::Transform>(projectileEntity) ||
            !ecs.hasComponent<components::Rigidbody>(projectileEntity)) {
            return;
        }

        auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);

        const auto owner = projectile.owner;
        const auto speedMultiplier = projectile.speedMultiplier;
        const auto payload = projectile.payload;
        const auto& transform = ecs.getComponent<components::Transform>(projectileEntity);
        const auto origin = transform.getPosition();
        const auto velocity = ecs.getComponent<components::Rigidbody>(projectileEntity).velocity;
        const auto direction = velocity.x == 0.f && velocity.y == 0.f
                                   ? utility::angleToVector(transform.getRotation())
                                   : velocity;
        const float angleRadians = utility::angleFromDirection(direction).asRadians();
        const auto payloadAngles = projectile.payloadFormation.calculateAngles(
            payload.size(), angleRadians);

        for (std::size_t index = 0; index < payload.size(); ++index) {
            const float payloadAngle = index < payloadAngles.size()
                                           ? payloadAngles[index]
                                           : angleRadians;
            spawnProjectile(world, owner, origin, payloadAngle, speedMultiplier, payload[index]);
        }
    }
}
