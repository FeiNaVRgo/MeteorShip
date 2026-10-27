#ifndef METEORSHIP_INVENTORY_H
#define METEORSHIP_INVENTORY_H

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "Spell.h"
#include "Turret.h"

namespace ms::ui {
    class UINode;
    namespace widgets {
        struct InventorySlotWidget;
        struct LabelWidget;
    }
}

namespace ms::inventory {
    inline constexpr std::size_t DefaultSpellSlotCount = 12;
    inline constexpr std::size_t DefaultTurretMountCount = 4;
    inline constexpr std::size_t ExampleTurretSpellSlotCount = 3;

    enum class SlotCollection {
        SpellInventory,
        TurretMount,
        TurretSpell
    };

    struct SlotAddress {
        SlotCollection collection;
        std::size_t index;
        std::size_t turretIndex = 0;

        bool operator==(const SlotAddress&) const = default;
    };

    enum class SlotClickAction {
        Empty,
        Selected,
        SelectionCleared,
        Moved,
        Swapped,
        Rejected
    };

    struct SlotClickResult {
        SlotClickAction action;
        std::string message;
    };

    class Inventory {
        struct SlotWidgetEntry {
            SlotAddress address;
            ui::UINode* node;
            ui::widgets::InventorySlotWidget* widget;
        };

        std::vector<std::optional<Spell>> spells;
        std::vector<std::optional<Turret>> turrets;
        std::optional<SlotAddress> selectedSlot;
        ui::UINode* uiRoot = nullptr;
        ui::widgets::LabelWidget* statusWidget = nullptr;
        std::vector<SlotWidgetEntry> slotWidgets;
        std::vector<std::vector<ui::UINode*>> turretColumnSlots;

        static constexpr float UiMargin = 12.f;
        static constexpr float UiHeaderHeight = 18.f;
        static constexpr float UiSlotSize = 34.f;
        static constexpr float UiGap = 4.f;
        static constexpr float UiStatusWidth = 600.f;

        ui::UINode& addLabel(std::string text, sf::Vector2f position, sf::Vector2f size);
        ui::UINode& addSlot(const SlotAddress& address, sf::Vector2f position);
        void ensureTurretColumnSlots();
        void refreshUI();
        void handleSlotClick(const SlotAddress& address);

        std::optional<Spell>* spellSlot(const SlotAddress& address);
        std::optional<Turret>* turretSlot(const SlotAddress& address);
        const std::optional<Spell>* spellSlot(const SlotAddress& address) const;
        const std::optional<Turret>* turretSlot(const SlotAddress& address) const;
        bool moveOrSwap(const SlotAddress& source, const SlotAddress& destination);

    public:
        explicit Inventory(std::size_t spellSlotCount = DefaultSpellSlotCount,
                           std::size_t turretMountCount = DefaultTurretMountCount);

        void buildUI(ui::UINode& root);

        std::size_t spellSlotCount() const { return spells.size(); }
        std::size_t turretMountCount() const { return turrets.size(); }
        std::size_t turretSpellSlotCount(std::size_t turretIndex) const;
        const Turret* turretAt(std::size_t turretIndex) const;

        bool setSpell(const SlotAddress& address, Spell spell);
        bool setTurret(std::size_t turretIndex, Turret turret);
        std::optional<SlotAddress> storeSpell(Spell spell);
        std::optional<SlotAddress> storeTurret(Turret turret);
        std::optional<Turret> takeTurret(std::size_t turretIndex);
        std::optional<std::size_t> addTurretSpellSlot(std::size_t turretIndex);
        bool removeTurretSpellSlot(std::size_t turretIndex, std::size_t slotIndex);
        bool resizeTurretSpellSlots(std::size_t turretIndex, std::size_t count);
        bool clear(const SlotAddress& address);
        bool hasItem(const SlotAddress& address) const;
        std::optional<std::string> itemName(const SlotAddress& address) const;
        const sf::Sprite* itemIcon(const SlotAddress& address) const;
        const std::optional<SlotAddress>& selectedAddress() const { return selectedSlot; }
        SlotClickResult clickSlot(const SlotAddress& address);
    };
}

#endif //METEORSHIP_INVENTORY_H
