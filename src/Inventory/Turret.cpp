#include "Turret.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Components/Drawable.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Utility/UtiFuncs.h"

namespace ms::inventory {
    namespace {
        void createProjectile(ecs::ECS& entityManager, const ecs::Entity owner,
                              const sf::Vector2f origin, const float angle,
                              const PayloadSpell& spell, const TurretStats& turretStats) {
            const auto direction = utility::directionFromAngle(sf::radians(angle));
            const auto projectile = entityManager.createEntity();

            components::Transform transform;
            transform.setPosition(origin);
            transform.setRotation(utility::rotationFromDirection(direction));
            entityManager.addComponent<components::Transform>(projectile, transform);

            auto sprite = spell.createProjectileSprite();
            sf::Vector2f projectileSize{};
            if (sprite) {
                sprite->sprite.setColor(spell.tint);
                projectileSize = sprite->sprite.getLocalBounds().size;
                components::Drawable drawable;
                drawable.sprite = std::move(sprite);
                entityManager.addComponent<components::Drawable>(projectile, std::move(drawable));
            }

            components::Rigidbody rigidbody;
            rigidbody.bounds = utility::centeredRect(origin, projectileSize);
            rigidbody.velocity = direction * spell.stats.speed * turretStats.speedMultiplier;
            entityManager.addComponent<components::Rigidbody>(projectile, rigidbody);

            float totalDamage = 0.f;
            for (const auto& damageAmount : spell.stats.damage) {
                totalDamage += damageAmount.amount;
            }
            entityManager.addComponent<components::Projectile>(
                projectile, components::Projectile{
                    .owner = owner,
                    .damage = totalDamage,
                    .damageTypes = spell.stats.damage,
                    .radius = spell.stats.radius,
                    .critChanceBonus = spell.stats.critChanceBonus,
                    .lifeTime = spell.stats.lifeTime,
                    .timerLifeTime = spell.stats.timerLifeTime,
                    .friendlyFire = spell.stats.friendlyFire,
                    .piercing = spell.stats.piercing,
                    .penetrating = spell.stats.penetrating,
                    .homing = spell.stats.homing,
                    .homingTurnRate = spell.stats.homingTurnRate,
                    .maxTravelDistance = spell.stats.maxTravelDistance
                });
            for (const auto& effect : spell.effects) {
                if (!entityManager.isEntityAlive(projectile)) break;
                if (!effect) continue;
                effect(entityManager, projectile);
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
        if (deckCursor >= deckOrder.size()) resetDeck();
        if (deckOrder.empty()) return;

        Payload payload;
        bool castSpell = false;
        float castDelay = stats.castDelay;
        const auto castLimit = std::max<std::size_t>(1, stats.spellsPerCast);
        const auto availableDraws = deckOrder.size() - deckCursor;
        auto drawLimit = std::min(castLimit, availableDraws);
        for (std::size_t drawn = 0; drawn < drawLimit; ++drawn) {
            const auto slotIndex = deckOrder[deckCursor++];
            if (slotIndex >= spellOrder.size() || !spellOrder[slotIndex]) continue;

            const auto spellEntity = *spellOrder[slotIndex];
            if (!entityManager.isEntityAlive(spellEntity) ||
                !entityManager.hasComponent<Spell>(spellEntity)) {
                continue;
            }

            auto& spell = entityManager.getComponent<Spell>(spellEntity);
            const auto& castingStats = spell.getCastingStats();
            const float manaCost = std::max(0.f, castingStats.manaCost);
            if (!spell.canCast() || mana < manaCost || !spell.consumeUse()) continue;

            mana -= manaCost;
            castDelay += castingStats.castDelay;
            cycleRechargeModifier += castingStats.rechargeTime;
            const auto multicastDraws = payload.addSpell(spell);
            const auto followingDraws = std::min(
                multicastDraws, availableDraws - (drawn + 1));
            drawLimit = std::max(drawLimit, drawn + 1 + followingDraws);
            castSpell = true;
        }

        const auto& payloadSpells = payload.getSpells();
        const float spreadCenter = (static_cast<float>(payloadSpells.size()) - 1.f) / 2.f;
        for (std::size_t index = 0; index < payloadSpells.size(); ++index) {
            const auto& payloadSpell = payloadSpells[index];
            const float spreadDegrees =
                (static_cast<float>(index) - spreadCenter) * stats.spread +
                payloadSpell.stats.spreadModification;
            const float projectileAngle =
                aimAngleRadians + sf::degrees(spreadDegrees).asRadians();
            createProjectile(entityManager, owner, origin, projectileAngle, payloadSpell, stats);
        }

        if (castSpell) castDelayRemaining = std::max(0.f, castDelay);
        if (deckCursor >= deckOrder.size()) {
            rechargeRemaining = std::max(0.f, stats.rechargeTime + cycleRechargeModifier);
            cycleRechargeModifier = 0.f;
        }
    }

    std::optional<ecs::Entity> Turret::spellEntityAt(const std::size_t index) const {
        if (index >= spellOrder.size()) return std::nullopt;
        return spellOrder[index];
    }

    bool Turret::setSpellEntity(const std::size_t index, const ecs::Entity spell) {
        if (index >= spellOrder.size()) return false;
        spellOrder[index] = spell;
        resetDeck();
        return true;
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
