#include "Turret.h"

#include <algorithm>
#include <utility>

namespace ms::inventory {
    Turret::Turret(std::string name, std::optional<sf::Sprite> icon, const std::size_t spellSlotCount)
        : name(std::move(name)), icon(std::move(icon)), spellOrder(spellSlotCount) {}

    const Spell* Turret::spellAt(const std::size_t index) const {
        if (index >= spellOrder.size() || !spellOrder[index]) return nullptr;
        return &*spellOrder[index];
    }

    bool Turret::setSpell(const std::size_t index, Spell spell) {
        if (index >= spellOrder.size()) return false;
        spellOrder[index] = std::move(spell);
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
