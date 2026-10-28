#ifndef METEORSHIP_TURRET_H
#define METEORSHIP_TURRET_H

#include <cstddef>
#include <optional>
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
    enum class TurretTargetMode {
        Automatic,
        Manual
    };

    class Turret {
        std::string name;
        std::optional<sf::Sprite> icon;
        std::vector<std::optional<ecs::Entity>> spellOrder;
        TurretTargetMode targetMode = TurretTargetMode::Automatic;
        float fireInterval = 0.35f;
        float cooldownRemaining = 0.f;

    public:
        explicit Turret(std::string name, std::optional<sf::Sprite> icon = std::nullopt,
                        std::size_t spellSlotCount = 0);

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
        std::size_t spellSlotCount() const { return spellOrder.size(); }
        TurretTargetMode getTargetMode() const { return targetMode; }
        void setTargetMode(TurretTargetMode mode) { targetMode = mode; }
        TurretTargetMode toggleTargetMode();
        void updateCooldown(float elapsedTime);
        bool readyToShoot() const { return cooldownRemaining <= 0.f; }
        bool shoot(ecs::ECS& entityManager, ecs::Entity owner, sf::Vector2f origin,
                   sf::Vector2f target, const sf::Texture& projectileTexture) const;
        void beginCooldown() { cooldownRemaining = fireInterval; }
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
