#include "Turret.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Components/Damage.h"
#include "Components/Drawable.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Utility/UtiFuncs.h"

namespace ms::inventory {
    namespace {
        void createProjectile(ecs::ECS& entityManager, const ecs::Entity owner,
                              const sf::Vector2f origin, const float angle,
                              const TurretStats& turretStats,
                              const ShootableSpell* spellInfo = nullptr) {
            auto entity = entityManager.createEntity();
            auto& tf = entityManager.addComponent<components::Transform>(entity);
            tf.setPosition(origin);
            const auto direction = utility::directionFromAngle(sf::radians(angle));
            tf.setRotation(utility::rotationFromDirection(direction));

            float speed = 500.f;
            if (spellInfo && spellInfo->finalStats.speed.has_value()) {
                speed = *spellInfo->finalStats.speed * turretStats.speedMultiplier;
            }

            auto& rb = entityManager.addComponent<components::Rigidbody>(entity);
            rb.velocity = direction * speed;

            // Default collision bounds for projectiles (e.g. 8x8)
            sf::Vector2f boundsSize(8.f, 8.f);

            auto& proj = entityManager.addComponent<components::Projectile>(entity);
            proj.owner = owner;
            if (spellInfo) {
                proj.lifeTime = spellInfo->finalStats.lifeTime;
                proj.friendlyFire = spellInfo->finalStats.friendlyFire;
                proj.piercing = spellInfo->finalStats.Piercing;
                proj.triggerType = spellInfo->finalStats.triggerType;
                proj.timer = spellInfo->finalStats.timerLifeTime.value_or(0.f);
                proj.payload = spellInfo->payload;
                proj.modifierHooks = spellInfo->modifierHooks;

                // Attach visual representation (Drawable) to projectile
                if (const auto* animatedSprite = spellInfo->spell.getProjectileSprite()) {
                    components::Drawable drawable;
                    drawable.sprite = animatedSprite->createInstance();
                    boundsSize = sf::Vector2f(static_cast<float>(animatedSprite->rect.size.x),
                                             static_cast<float>(animatedSprite->rect.size.y));
                    entityManager.addComponent<components::Drawable>(entity, std::move(drawable));
                } else if (const auto* icon = spellInfo->spell.getIcon()) {
                    components::Drawable drawable;
                    drawable.sprite.emplace(icon->getTexture(), icon->getTextureRect(), 1, std::nullopt);
                    boundsSize = sf::Vector2f(static_cast<float>(icon->getTextureRect().size.x),
                                             static_cast<float>(icon->getTextureRect().size.y));
                    entityManager.addComponent<components::Drawable>(entity, std::move(drawable));
                }

                // Run onInit hooks (one-time initialization)
                ModifierContext ctx{
                    .entityManager = entityManager,
                    .projectileEntity = entity,
                    .targetEntity = 0,
                    .hitNormal = {},
                    .deltaTime = 0.f
                };
                for (const auto& hook : proj.modifierHooks) {
                    if (hook.onInit) {
                        hook.onInit(ctx);
                    }
                }
            }

            // Set initial centered bounds based on sprite dimensions
            rb.bounds = utility::centeredRect(origin, boundsSize);

            auto& dmg = entityManager.addComponent<components::DamageAmount>(entity);
            dmg.type = components::DamageType::Projectile;
            if (spellInfo && spellInfo->finalStats.damage.has_value()) {
                dmg.amount = *spellInfo->finalStats.damage;
            }
        }
    }

    Turret::Turret(std::string name, std::optional<sf::Sprite> icon,
                   const std::size_t spellSlotCount, const TurretStats& stats)
        : name(std::move(name)),
          icon(std::move(icon)),
          spellOrder(std::min(spellSlotCount, stats.capacity)),
          stats(stats),
          mana(this->stats.manaMax) {
        resetDeck();
    }

    TurretTargetMode Turret::toggleTargetMode() {
        targetMode = targetMode == TurretTargetMode::Automatic
            ? TurretTargetMode::Manual
            : TurretTargetMode::Automatic;
        return targetMode;
    }

    void Turret::updateCooldown(const float elapsedTime) {
        castDelayRemaining = std::max(0.f, castDelayRemaining - elapsedTime);
        if (rechargeRemaining > 0.f) {
            rechargeRemaining = std::max(0.f, rechargeRemaining - elapsedTime);
            if (rechargeRemaining == 0.f) resetDeck();
        }
        mana = std::min(stats.manaMax, mana + stats.manaChargeSpeed * elapsedTime);
    }

    void Turret::resetDeck() {
        deckOrder.clear();
        deckOrder.reserve(spellOrder.size());
        for (std::size_t index = 0; index < spellOrder.size(); ++index) {
            if (spellOrder[index]) deckOrder.push_back(index);
        }
        if (stats.shuffle) std::ranges::shuffle(deckOrder, randomEngine);
        deckCursor = 0;
        cycleRechargeModifier = 0.f;
    }

    void Turret::shoot(ecs::ECS& entityManager, const ecs::Entity owner,
                       const sf::Vector2f origin, const float aimAngleRadians) {
        if (!readyToShoot()) return;

        Payload payload(this);
        auto& vec_payloads = payload.gatherPayloadedSpells();

        for (const auto& payloadGroup : vec_payloads) {
            const auto& spells = payloadGroup.resolvedSpells;
            if (spells.empty()) continue;

            auto angles = payloadGroup.formation.calculateAngles(spells.size(), aimAngleRadians);
            for (std::size_t i = 0; i < spells.size(); ++i) {
                float angle = (i < angles.size()) ? angles[i] : aimAngleRadians;
                createProjectile(entityManager, owner, origin, angle, stats, &spells[i]);
            }
        }
    }

    std::size_t Turret::addSpellSlot() {
        if (spellOrder.size() >= stats.capacity) return spellOrder.size();
        spellOrder.emplace_back(std::nullopt);
        resetDeck();
        return spellOrder.size() - 1;
    }

    bool Turret::removeSpellSlot(const std::size_t index) {
        if (index >= spellOrder.size() || spellOrder[index]) return false;
        spellOrder.erase(spellOrder.begin() + static_cast<std::ptrdiff_t>(index));
        resetDeck();
        return true;
    }

    bool Turret::resizeSpellSlots(const std::size_t count) {
        if (count > stats.capacity) return false;
        if (count < spellOrder.size() &&
            std::any_of(spellOrder.begin() + static_cast<std::ptrdiff_t>(count), spellOrder.end(),
                        [](const auto& spell) { return spell.has_value(); })) {
            return false;
        }
        spellOrder.resize(count);
        resetDeck();
        return true;
    }
}
