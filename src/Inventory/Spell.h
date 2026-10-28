#ifndef METEORSHIP_SPELL_H
#define METEORSHIP_SPELL_H

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "Components/Damage.h"
#include "Utility/AnimatedSprite.h"

namespace ms::inventory {
    struct SpellStats {
        std::size_t uses = std::numeric_limits<std::size_t>::max();
        float castDelay = 0.f;
        float rechargeTime = 0.f;
        float manaCost = 1.f;
        float spreadModification = 0.f;
        float radius = 0.f;
        float speed = 260.f;
        float critChanceBonus = 0.f;
        std::vector<components::DamageAmount> damage{
            {.type = components::DamageType::Projectile, .amount = 1.f}
        };
    };

    class Spell {
        std::string name;
        std::optional<sf::Sprite> icon;
        std::optional<utility::AnimatedSprite> projectileAnimation;
        SpellStats stats;
        sf::Color projectileTint = sf::Color::White;
        std::size_t usesRemaining;

    public:
        explicit Spell(std::string name, std::optional<sf::Sprite> icon = std::nullopt,
                       SpellStats stats = {},
                       std::optional<utility::AnimatedSprite> projectileAnimation = std::nullopt,
                       const sf::Color projectileTint = sf::Color::White)
            : name(std::move(name)),
              icon(std::move(icon)),
              projectileAnimation(std::move(projectileAnimation)),
              stats(std::move(stats)),
              projectileTint(projectileTint),
              usesRemaining(this->stats.uses) {}

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
        std::optional<utility::AnimatedSprite> createProjectileSprite() const {
            if (projectileAnimation) return projectileAnimation->createInstance();
            if (!icon) return std::nullopt;
            return utility::AnimatedSprite(
                icon->getTexture(), icon->getTextureRect(), 1, std::nullopt);
        }
        const SpellStats& getStats() const { return stats; }
        sf::Color getProjectileTint() const { return projectileTint; }
        bool canCast() const { return usesRemaining > 0; }

        bool consumeUse() {
            if (!canCast()) return false;
            if (usesRemaining != std::numeric_limits<std::size_t>::max()) --usesRemaining;
            return true;
        }
    };
}

#endif //METEORSHIP_SPELL_H
