#ifndef METEORSHIP_TURRET_H
#define METEORSHIP_TURRET_H

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>

#include "Spell.h"

namespace ms::inventory {
    class Turret {
        std::string name;
        std::optional<sf::Sprite> icon;
        std::vector<std::optional<Spell>> spellOrder;

    public:
        explicit Turret(std::string name, std::optional<sf::Sprite> icon = std::nullopt,
                        std::size_t spellSlotCount = 0);

        const std::string& getName() const { return name; }
        const sf::Sprite* getIcon() const { return icon ? &*icon : nullptr; }
        std::size_t spellSlotCount() const { return spellOrder.size(); }
        const Spell* spellAt(std::size_t index) const;
        bool setSpell(std::size_t index, Spell spell);
        std::size_t addSpellSlot();
        bool removeSpellSlot(std::size_t index);
        bool resizeSpellSlots(std::size_t count);

    private:
        friend class Inventory;
    };
}

#endif //METEORSHIP_TURRET_H
