#ifndef METEORSHIP_TURRET_H
#define METEORSHIP_TURRET_H

#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>

#include "ECS/ECS.h"


namespace ms::inventory {

}

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
        using SpellsOrder = std::vector<std::optional<ecs::Entity>>;
        using DeckOrder   = std::vector<std::size_t>;

        std::string name;
        std::optional<sf::Sprite> icon;
        SpellsOrder spellOrder;// for ui
        TurretStats stats;
        float mana;
        float castDelayRemaining = 0.f;
        float rechargeRemaining = 0.f;
        float cycleRechargeModifier = 0.f;
        DeckOrder deckOrder;
        std::size_t deckCursor = 0;
        std::mt19937 randomEngine{std::random_device{}()}; //this is bs btw generation of random  numbers should be delegated to its own class (singleton i think)
        TurretTargetMode targetMode = TurretTargetMode::Automatic;

    public:

        class TurretIterator {
            SpellsOrder* spells = nullptr;
            const DeckOrder* deck = nullptr;
            std::size_t index = 0;

            TurretIterator(SpellsOrder* spells, const DeckOrder* deck, const std::size_t index)
                : spells(spells), deck(deck), index(index) {}

            friend class Turret;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = ecs::Entity;
            using difference_type = std::ptrdiff_t;
            using reference = ecs::Entity&;
            using pointer = ecs::Entity*;

            TurretIterator() = default;

            reference operator*() const { return *(*spells)[(*deck)[index]]; }
            pointer operator->() const { return &operator*(); }

            TurretIterator& operator++() {
                ++index;
                return *this;
            }

            TurretIterator operator++(int) {
                TurretIterator previous = *this;
                ++*this;
                return previous;
            }

            bool operator==(const TurretIterator&) const = default;
        };

        explicit Turret(std::string name, std::optional<sf::Sprite> icon = std::nullopt,
                        std::size_t spellSlotCount = 0, const TurretStats& stats = {});

        TurretIterator begin() { return {&spellOrder, &deckOrder, 0}; }
        TurretIterator end() { return {&spellOrder, &deckOrder, deckOrder.size()}; }

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
        void shoot(ecs::ECS& entityManager, ecs::Entity owner, sf::Vector2f origin,
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
