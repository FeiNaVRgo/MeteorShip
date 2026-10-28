#ifndef METEORSHIP_TURRET_H
#define METEORSHIP_TURRET_H

#include <cstddef>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>

#include "Components/Projectile.h"
#include "Components/Drawable.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "Spell.h"

namespace ms::inventory {
    struct TurretStats {
        bool shuffle = false;
        std::size_t spellsPerCast = 1;
        float castDelay = 0.f;
        float rechargeTime = 0.35f;
        float manaMax = 100.f;
        float manaChargeSpeed = 10.f;
        std::size_t capacity = std::numeric_limits<std::size_t>::max();
        float spread = 0.f;
        float speedMultiplier = 1.f;
    };

    enum class TurretTargetMode {
        Automatic,
        Manual
    };

    class Turret {
        std::string name;
        std::optional<sf::Sprite> icon;
        std::vector<std::optional<ecs::Entity>> spellOrder;
        TurretStats stats;
        float mana;
        float castDelayRemaining = 0.f;
        float rechargeRemaining = 0.f;
        float cycleRechargeModifier = 0.f;
        std::vector<std::size_t> deckOrder;
        std::size_t deckCursor = 0;
        std::mt19937 randomEngine{std::random_device{}()}; //this is bs btw generation of random  numbers should be delegated to its own class (singleton i think)
        TurretTargetMode targetMode = TurretTargetMode::Automatic;

    public:
        explicit Turret(std::string name, std::optional<sf::Sprite> icon = std::nullopt,
                        std::size_t spellSlotCount = 0, const TurretStats& stats = {});

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
        std::size_t spellSlotCount() const { return spellOrder.size(); }
        std::size_t capacity() const { return stats.capacity; }
        const TurretStats& getStats() const { return stats; }
        float getMana() const { return mana; }
        TurretTargetMode getTargetMode() const { return targetMode; }
        void setTargetMode(TurretTargetMode mode) { targetMode = mode; }
        TurretTargetMode toggleTargetMode();
        void updateCooldown(float elapsedTime);
        bool readyToShoot() const {
            return castDelayRemaining <= 0.f && rechargeRemaining <= 0.f;
        }
        std::size_t shoot(ecs::ECS& entityManager, ecs::Entity owner, sf::Vector2f origin,
                          float aimAngleRadians);
        void resetDeck();
        std::optional<ecs::Entity> spellEntityAt(std::size_t index) const;
        bool setSpellEntity(std::size_t index, ecs::Entity spell);
        std::size_t addSpellSlot();
        bool removeSpellSlot(std::size_t index);
        bool resizeSpellSlots(std::size_t count);

    private:
        friend class Inventory;
    };
}

#endif //METEORSHIP_TURRET_H
