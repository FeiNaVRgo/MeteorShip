#include "Turret.h"

#include <algorithm>
#include <utility>

#include "Components/Drawable.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"

namespace ms::inventory {
    namespace {
        void createProjectile(ecs::ECS& entityManager, const ecs::Entity owner,
                              const sf::Vector2f origin, const float angle, const TurretStats& turretStats) {
            //TODO
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
        //TODO
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
