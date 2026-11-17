#include "Turret.h"
#include "ECS/World.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Systems/Projectiles/ProjectileSpawner.h"

namespace ms::inventory {
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

    void Turret::shoot(World& world, const ecs::Entity owner,
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
                ms::systems::spawnProjectile(
                    world, owner, origin, angle, stats.speedMultiplier, spells[i]);
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
