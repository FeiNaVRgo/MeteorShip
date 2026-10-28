#include "Turret.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Utility/AnimatedSprite.h"

namespace ms::inventory {
    Turret::Turret(std::string name, std::optional<sf::Sprite> icon, const std::size_t spellSlotCount)
        : name(std::move(name)), icon(std::move(icon)), spellOrder(spellSlotCount) {}

    TurretTargetMode Turret::toggleTargetMode() {
        targetMode = targetMode == TurretTargetMode::Automatic
            ? TurretTargetMode::Manual
            : TurretTargetMode::Automatic;
        return targetMode;
    }

    void Turret::updateCooldown(const float elapsedTime) {
        cooldownRemaining = std::max(0.f, cooldownRemaining - elapsedTime);
    }

    //temp way of doing it
    bool Turret::shoot(ecs::ECS& entityManager, const ecs::Entity owner, const sf::Vector2f origin,
                       const sf::Vector2f target, const sf::Texture& projectileTexture) const {
        const auto displacement = target - origin;
        const float distance = std::sqrt(displacement.x * displacement.x + displacement.y * displacement.y);
        if (distance <= 0.001f) return false;
        const sf::Vector2f direction = displacement / distance;

        float damage = 1.f;
        float speed = 260.f;
        sf::Color tint = sf::Color::White;
        for (const auto& spellEntity : spellOrder) {
            if (!spellEntity || !entityManager.isEntityAlive(*spellEntity) ||
                !entityManager.hasComponent<Spell>(*spellEntity)) {
                continue;
            }
            entityManager.getComponent<Spell>(*spellEntity).applyTo(damage, speed, tint);
        }

        constexpr sf::Vector2i ProjectileSize{8, 8};
        const auto projectile = entityManager.createEntity();
        std::cout << projectile << std::endl;

        components::Transform transform;
        transform.setPosition(origin);
        transform.setRotation(sf::radians(std::atan2(direction.x, -direction.y)));
        entityManager.addComponent<components::Transform>(projectile, transform);

        utility::AnimatedSprite sprite(
            projectileTexture,
            sf::IntRect({0, 0}, ProjectileSize),
            1,
            std::nullopt
        );
        sprite.sprite.setColor(tint);
        components::Drawable drawable;
        drawable.sprite = std::move(sprite);
        entityManager.addComponent<components::Drawable>(projectile, drawable);

        components::Rigidbody rigidbody;
        const auto localBounds = drawable.sprite->sprite.getLocalBounds();
        rigidbody.bounds = sf::FloatRect(origin - localBounds.size / 2.f, localBounds.size);
        rigidbody.velocity = direction * speed;
        entityManager.addComponent<components::Rigidbody>(projectile, rigidbody);
        entityManager.addComponent<components::Projectile>(projectile, components::Projectile{owner, damage});
        return true;
    }

    std::optional<ecs::Entity> Turret::spellEntityAt(const std::size_t index) const {
        if (index >= spellOrder.size()) return std::nullopt;
        return spellOrder[index];
    }

    bool Turret::setSpellEntity(const std::size_t index, const ecs::Entity spell) {
        if (index >= spellOrder.size()) return false;
        spellOrder[index] = spell;
        return true;
    }

    std::size_t Turret::addSpellSlot() {
        spellOrder.emplace_back(std::nullopt);
        return spellOrder.size() - 1;
    }

    bool Turret::removeSpellSlot(const std::size_t index) {
        if (index >= spellOrder.size() || spellOrder[index]) return false;
        spellOrder.erase(spellOrder.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    bool Turret::resizeSpellSlots(const std::size_t count) {
        if (count < spellOrder.size() &&
            std::any_of(spellOrder.begin() + static_cast<std::ptrdiff_t>(count), spellOrder.end(),
                        [](const auto& spell) { return spell.has_value(); })) {
            return false;
        }
        spellOrder.resize(count);
        return true;
    }
}
