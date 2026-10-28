#include "Turret.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ms::inventory {
    namespace {
        sf::Vector2f directionFromAngle(const float angle) {
            return {std::cos(angle), std::sin(angle)};
        }

        void createProjectile(ecs::ECS& entityManager, const ecs::Entity owner,
                              const sf::Vector2f origin, const float angle,
                              const Spell& spell, const TurretStats& turretStats) {
            const auto direction = directionFromAngle(angle);
            const auto projectile = entityManager.createEntity();

            components::Transform transform;
            transform.setPosition(origin);
            transform.setRotation(sf::radians(angle + sf::degrees(90.f).asRadians()));
            entityManager.addComponent<components::Transform>(projectile, transform);

            auto sprite = spell.createProjectileSprite();
            sf::Vector2f projectileSize{};
            if (sprite) {
                sprite->sprite.setColor(spell.getProjectileTint());
                projectileSize = sprite->sprite.getLocalBounds().size;
                components::Drawable drawable;
                drawable.sprite = std::move(sprite);
                entityManager.addComponent<components::Drawable>(projectile, std::move(drawable));
            }

            components::Rigidbody rigidbody;
            rigidbody.bounds = sf::FloatRect(origin - projectileSize / 2.f, projectileSize);
            rigidbody.velocity = direction * spell.getStats().speed * turretStats.speedMultiplier;
            entityManager.addComponent<components::Rigidbody>(projectile, rigidbody);

            const auto& spellStats = spell.getStats();
            float totalDamage = 0.f;
            for (const auto& damageAmount : spellStats.damage) {
                totalDamage += damageAmount.amount;
            }
            entityManager.addComponent<components::Projectile>(
                projectile, components::Projectile{
                    .owner = owner,
                    .damage = totalDamage,
                    .damageTypes = spellStats.damage,
                    .radius = spellStats.radius,
                    .critChanceBonus = spellStats.critChanceBonus
                });
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

    std::size_t Turret::shoot(ecs::ECS& entityManager, const ecs::Entity owner,
                              const sf::Vector2f origin, const float aimAngleRadians) {
        if (!readyToShoot()) return 0;
        if (deckCursor >= deckOrder.size()) resetDeck();
        if (deckOrder.empty()) return 0;

        struct PreparedSpell {
            ecs::Entity entity;
            float manaCost;
        };
        std::vector<PreparedSpell> castSpells;
        float availableMana = mana;
        const auto castLimit = std::max<std::size_t>(1, stats.spellsPerCast);
        for (std::size_t drawn = 0; drawn < castLimit && deckCursor < deckOrder.size(); ++drawn) {
            const auto slotIndex = deckOrder[deckCursor++];
            if (slotIndex >= spellOrder.size() || !spellOrder[slotIndex]) continue;

            const auto spellEntity = *spellOrder[slotIndex];
            if (!entityManager.isEntityAlive(spellEntity) ||
                !entityManager.hasComponent<Spell>(spellEntity) ||
                std::ranges::any_of(castSpells, [spellEntity](const PreparedSpell& prepared) {
                    return prepared.entity == spellEntity;
                })) continue;

            const auto& spell = entityManager.getComponent<Spell>(spellEntity);
            const float manaCost = std::max(0.f, spell.getStats().manaCost);
            if (!spell.canCast() || availableMana < manaCost) continue;
            availableMana -= manaCost;
            castSpells.push_back({spellEntity, manaCost});
        }

        const float center = (static_cast<float>(castSpells.size()) - 1.f) / 2.f;
        float castDelay = stats.castDelay;
        std::size_t firedCount = 0;
        for (std::size_t index = 0; index < castSpells.size(); ++index) {
            const auto [spellEntity, manaCost] = castSpells[index];
            if (!entityManager.isEntityAlive(spellEntity) ||
                !entityManager.hasComponent<Spell>(spellEntity)) continue;

            auto& spell = entityManager.getComponent<Spell>(spellEntity);
            if (!spell.consumeUse()) continue;
            mana -= manaCost;

            const auto& spellStats = spell.getStats();
            const float spreadDegrees =
                (static_cast<float>(index) - center) * stats.spread +
                spellStats.spreadModification;
            const float projectileAngle = aimAngleRadians + sf::degrees(spreadDegrees).asRadians();
            createProjectile(entityManager, owner, origin, projectileAngle, spell, stats);
            castDelay += spellStats.castDelay;
            cycleRechargeModifier += spellStats.rechargeTime;
            ++firedCount;
        }

        if (firedCount > 0) castDelayRemaining = std::max(0.f, castDelay);
        if (deckCursor >= deckOrder.size()) {
            rechargeRemaining = std::max(0.f, stats.rechargeTime + cycleRechargeModifier);
            cycleRechargeModifier = 0.f;
        }
        return firedCount;
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
