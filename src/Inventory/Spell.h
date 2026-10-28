#ifndef METEORSHIP_SPELL_H
#define METEORSHIP_SPELL_H

#include <optional>
#include <string>
#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace ms::inventory {
    class Spell {
        std::string name;
        std::optional<sf::Sprite> icon;
        float damageMultiplier = 1.f;
        float projectileSpeedMultiplier = 1.f;
        sf::Color projectileTint = sf::Color::White;

    public:
        explicit Spell(std::string name, std::optional<sf::Sprite> icon = std::nullopt,
                       const float damageMultiplier = 1.f,
                       const float projectileSpeedMultiplier = 1.f,
                       const sf::Color projectileTint = sf::Color::White)
            : name(std::move(name)),
              icon(std::move(icon)),
              damageMultiplier(damageMultiplier),
              projectileSpeedMultiplier(projectileSpeedMultiplier),
              projectileTint(projectileTint) {}

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }

        void applyTo(float& damage, float& speed, sf::Color& tint) const {
            damage *= damageMultiplier;
            speed *= projectileSpeedMultiplier;
            if (projectileTint != sf::Color::White) tint = projectileTint;
        }
    };
}

#endif //METEORSHIP_SPELL_H
