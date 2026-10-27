#ifndef METEORSHIP_SPELL_H
#define METEORSHIP_SPELL_H

#include <optional>
#include <string>
#include <utility>

#include <SFML/Graphics/Sprite.hpp>

namespace ms::inventory {
    class Spell {
        std::string name;
        std::optional<sf::Sprite> icon;

    public:
        explicit Spell(std::string name, std::optional<sf::Sprite> icon = std::nullopt)
            : name(std::move(name)), icon(std::move(icon)) {}

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
    };
}

#endif //METEORSHIP_SPELL_H
