#include "Inventory.h"

#include <utility>

#include "UI/UINode.h"

namespace ms::inventory {
    Inventory::Inventory(const std::size_t spellSlotCount, const std::size_t turretMountCount)
        : spells(spellSlotCount), turrets(turretMountCount) {}

    ui::UINode& Inventory::addLabel(std::string text, const sf::Vector2f position, const sf::Vector2f size) {
        auto& node = uiRoot->add(ui::widgets::LabelWidget{.text = std::move(text)}, size);
        node.alignToParent(ui::Alignment::TopLeft, position);
        return node;
    }

    ui::UINode& Inventory::addSlot(const SlotAddress& address, const sf::Vector2f position) {
        auto& node = uiRoot->add(ui::widgets::InventorySlotWidget{}, {UiSlotSize, UiSlotSize});
        node.alignToParent(ui::Alignment::TopLeft, position);
        auto* widget = node.getWidget<ui::widgets::InventorySlotWidget>();
        widget->onClick = [this, address] { handleSlotClick(address); };
        slotWidgets.push_back({address, &node, widget});
        return node;
    }

    void Inventory::buildUI(ui::UINode& root) {
        uiRoot = &root;
        slotWidgets.clear();
        turretColumnSlots.clear();

        addLabel("TURRETS", {UiMargin, UiMargin}, {UiSlotSize * 2.f + UiGap, UiHeaderHeight});

        const auto spellRowWidth = spells.empty()
            ? 0.f
            : static_cast<float>(spells.size()) * (UiSlotSize + UiGap) - UiGap;
        addLabel("SPELLS", {0.f, UiMargin}, {spellRowWidth, UiHeaderHeight})
            .alignToParent(ui::Alignment::TopRight, {-UiMargin, UiMargin});

        const auto slotY = UiMargin + UiHeaderHeight + UiGap;
        for (std::size_t i = 0; i < turrets.size(); ++i) {
            const auto x = UiMargin + static_cast<float>(i) * (UiSlotSize + UiGap);
            addSlot({SlotCollection::TurretMount, i}, {x, slotY});
        }

        for (std::size_t i = 0; i < spells.size(); ++i) {
            auto& slot = addSlot({SlotCollection::SpellInventory, i}, {0.f, slotY});
            slot.alignToParent(
                ui::Alignment::TopRight,
                {-UiMargin - static_cast<float>(spells.size() - i - 1) * (UiSlotSize + UiGap), slotY});
        }

        turretColumnSlots.resize(turrets.size());

        auto& statusNode = addLabel(
            "Click an item, then a destination slot.", {UiMargin, 0.f}, {UiStatusWidth, UiHeaderHeight});
        statusNode.alignToParent(ui::Alignment::BottomLeft, {UiMargin, -UiMargin});
        statusWidget = statusNode.getWidget<ui::widgets::LabelWidget>();

        ensureTurretColumnSlots();
        refreshUI();
    }

    void Inventory::ensureTurretColumnSlots() {
        if (!uiRoot) return;
        for (std::size_t turretIndex = 0; turretIndex < turretColumnSlots.size(); ++turretIndex) {
            auto& nodes = turretColumnSlots[turretIndex];
            const auto desiredCount = turretSpellSlotCount(turretIndex);
            while (nodes.size() < desiredCount) {
                const auto index = nodes.size();
                const auto x = UiMargin + static_cast<float>(turretIndex) * (UiSlotSize + UiGap);
                const auto y = UiMargin + UiHeaderHeight + UiGap + UiSlotSize + UiGap +
                    static_cast<float>(index) * (UiSlotSize + UiGap);
                nodes.push_back(&addSlot(
                    {SlotCollection::TurretSpell, index, turretIndex}, {x, y}));
            }
        }
    }

    void Inventory::refreshUI() {
        if (!uiRoot) return;
        ensureTurretColumnSlots();

        for (const auto& entry : slotWidgets) {
            const auto& address = entry.address;
            auto* widget = entry.widget;
            widget->visible = address.collection != SlotCollection::TurretSpell ||
                (address.turretIndex < turrets.size() && turrets[address.turretIndex] &&
                 address.index < turretSpellSlotCount(address.turretIndex));
            widget->itemName = itemName(address).value_or("");
            if (const auto* icon = itemIcon(address)) {
                widget->icon = *icon;
            } else {
                widget->icon.reset();
            }
            widget->selected = selectedSlot && *selectedSlot == address;
        }

    }

    void Inventory::handleSlotClick(const SlotAddress& address) {
        const auto result = clickSlot(address);
        if (statusWidget) statusWidget->text = result.message;
    }

    std::optional<Spell>* Inventory::spellSlot(const SlotAddress& address) {
        switch (address.collection) {
            case SlotCollection::SpellInventory:
                return address.index < spells.size() ? &spells[address.index] : nullptr;
            case SlotCollection::TurretSpell:
                if (address.turretIndex >= turrets.size() || !turrets[address.turretIndex] ||
                    address.index >= turrets[address.turretIndex]->spellOrder.size()) {
                    return nullptr;
                }
                return &turrets[address.turretIndex]->spellOrder[address.index];
            case SlotCollection::TurretMount:
                return nullptr;
        }
        return nullptr;
    }

    std::optional<Turret>* Inventory::turretSlot(const SlotAddress& address) {
        if (address.collection != SlotCollection::TurretMount || address.index >= turrets.size()) {
            return nullptr;
        }
        return &turrets[address.index];
    }

    const std::optional<Spell>* Inventory::spellSlot(const SlotAddress& address) const {
        switch (address.collection) {
            case SlotCollection::SpellInventory:
                return address.index < spells.size() ? &spells[address.index] : nullptr;
            case SlotCollection::TurretSpell:
                if (address.turretIndex >= turrets.size() || !turrets[address.turretIndex] ||
                    address.index >= turrets[address.turretIndex]->spellOrder.size()) {
                    return nullptr;
                }
                return &turrets[address.turretIndex]->spellOrder[address.index];
            case SlotCollection::TurretMount:
                return nullptr;
        }
        return nullptr;
    }

    const std::optional<Turret>* Inventory::turretSlot(const SlotAddress& address) const {
        if (address.collection != SlotCollection::TurretMount || address.index >= turrets.size()) {
            return nullptr;
        }
        return &turrets[address.index];
    }

    std::size_t Inventory::turretSpellSlotCount(const std::size_t turretIndex) const {
        if (turretIndex >= turrets.size() || !turrets[turretIndex]) {
            return 0;
        }
        return turrets[turretIndex]->spellSlotCount();
    }

    const Turret* Inventory::turretAt(const std::size_t turretIndex) const {
        if (turretIndex >= turrets.size() || !turrets[turretIndex]) return nullptr;
        return &*turrets[turretIndex];
    }

    bool Inventory::setSpell(const SlotAddress& address, Spell spell) {
        auto* slot = spellSlot(address);
        if (!slot) return false;
        *slot = std::move(spell);
        refreshUI();
        return true;
    }

    bool Inventory::setTurret(const std::size_t turretIndex, Turret turret) {
        if (turretIndex >= turrets.size() || turrets[turretIndex]) return false;
        turrets[turretIndex] = std::move(turret);
        refreshUI();
        return true;
    }

    std::optional<SlotAddress> Inventory::storeSpell(Spell spell) {
        for (std::size_t i = 0; i < spells.size(); ++i) {
            if (spells[i]) continue;
            spells[i] = std::move(spell);
            refreshUI();
            return SlotAddress{SlotCollection::SpellInventory, i};
        }
        return std::nullopt;
    }

    std::optional<SlotAddress> Inventory::storeTurret(Turret turret) {
        for (std::size_t i = 0; i < turrets.size(); ++i) {
            if (turrets[i]) continue;
            turrets[i] = std::move(turret);
            refreshUI();
            return SlotAddress{SlotCollection::TurretMount, i};
        }
        return std::nullopt;
    }

    std::optional<Turret> Inventory::takeTurret(const std::size_t turretIndex) {
        if (turretIndex >= turrets.size() || !turrets[turretIndex]) return std::nullopt;
        auto removed = std::move(turrets[turretIndex]);
        turrets[turretIndex].reset();
        if (selectedSlot &&
            ((selectedSlot->collection == SlotCollection::TurretMount && selectedSlot->index == turretIndex) ||
             (selectedSlot->collection == SlotCollection::TurretSpell &&
              selectedSlot->turretIndex == turretIndex))) {
            selectedSlot.reset();
        }
        refreshUI();
        return removed;
    }

    std::optional<std::size_t> Inventory::addTurretSpellSlot(const std::size_t turretIndex) {
        if (turretIndex >= turrets.size() || !turrets[turretIndex]) return std::nullopt;
        const auto addedSlot = turrets[turretIndex]->addSpellSlot();
        refreshUI();
        return addedSlot;
    }

    bool Inventory::removeTurretSpellSlot(const std::size_t turretIndex, const std::size_t slotIndex) {
        if (turretIndex >= turrets.size() || !turrets[turretIndex]) return false;
        if (!turrets[turretIndex]->removeSpellSlot(slotIndex)) return false;
        if (selectedSlot && selectedSlot->collection == SlotCollection::TurretSpell &&
            selectedSlot->turretIndex == turretIndex) {
            if (selectedSlot->index == slotIndex) {
                selectedSlot.reset();
            } else if (selectedSlot->index > slotIndex) {
                --selectedSlot->index;
            }
        }
        refreshUI();
        return true;
    }

    bool Inventory::resizeTurretSpellSlots(const std::size_t turretIndex, const std::size_t count) {
        if (turretIndex >= turrets.size() || !turrets[turretIndex]) return false;
        const auto oldCount = turrets[turretIndex]->spellSlotCount();
        if (!turrets[turretIndex]->resizeSpellSlots(count)) return false;
        if (selectedSlot && selectedSlot->collection == SlotCollection::TurretSpell &&
            selectedSlot->turretIndex == turretIndex && selectedSlot->index >= count && count < oldCount) {
            selectedSlot.reset();
        }
        refreshUI();
        return true;
    }

    bool Inventory::moveOrSwap(const SlotAddress& source, const SlotAddress& destination) {
        if (source == destination) return false;

        const bool sourceIsSpell = source.collection != SlotCollection::TurretMount;
        if (sourceIsSpell != (destination.collection != SlotCollection::TurretMount)) return false;

        if (sourceIsSpell) {
            auto* sourceSlot = spellSlot(source);
            auto* destinationSlot = spellSlot(destination);
            if (!sourceSlot || !destinationSlot || !*sourceSlot) return false;
            sourceSlot->swap(*destinationSlot);
            return true;
        }

        auto* sourceSlot = turretSlot(source);
        auto* destinationSlot = turretSlot(destination);
        if (!sourceSlot || !destinationSlot || !*sourceSlot) return false;
        sourceSlot->swap(*destinationSlot);
        return true;
    }

    bool Inventory::clear(const SlotAddress& address) {
        if (address.collection == SlotCollection::TurretMount) return false;
        if (auto* slot = spellSlot(address)) {
            slot->reset();
            if (selectedSlot && *selectedSlot == address) selectedSlot.reset();
            refreshUI();
            return true;
        }
        return false;
    }

    bool Inventory::hasItem(const SlotAddress& address) const {
        if (const auto* slot = spellSlot(address)) return slot->has_value();
        if (const auto* slot = turretSlot(address)) return slot->has_value();
        return false;
    }

    std::optional<std::string> Inventory::itemName(const SlotAddress& address) const {
        if (const auto* slot = spellSlot(address); slot && *slot) return slot->value().getName();
        if (const auto* slot = turretSlot(address); slot && *slot) return slot->value().getName();
        return std::nullopt;
    }

    const sf::Sprite* Inventory::itemIcon(const SlotAddress& address) const {
        if (const auto* slot = spellSlot(address); slot && *slot && slot->value().getIcon()) {
            return slot->value().getIcon();
        }
        if (const auto* slot = turretSlot(address); slot && *slot && slot->value().getIcon()) {
            return slot->value().getIcon();
        }
        return nullptr;
    }

    SlotClickResult Inventory::clickSlot(const SlotAddress& address) {
        if (!hasItem(address) && !spellSlot(address) && !turretSlot(address)) {
            return {SlotClickAction::Rejected, "That slot is not available."};
        }

        if (!selectedSlot) {
            if (!hasItem(address)) return {SlotClickAction::Empty, "That slot is empty."};
            selectedSlot = address;
            refreshUI();
            return {SlotClickAction::Selected, "Selected " + itemName(address).value_or("item") + "."};
        }

        if (*selectedSlot == address) {
            selectedSlot.reset();
            refreshUI();
            return {SlotClickAction::SelectionCleared, "Selection cleared."};
        }

        const auto source = *selectedSlot;
        const auto sourceName = itemName(source).value_or("item");
        const auto destinationName = itemName(address);
        const bool swapped = destinationName.has_value();
        if (!moveOrSwap(source, address)) {
            return {SlotClickAction::Rejected, "Those slots cannot exchange these items."};
        }

        selectedSlot.reset();
        refreshUI();
        if (swapped) {
            return {SlotClickAction::Swapped, "Swapped " + sourceName + " and " + *destinationName + "."};
        }
        return {SlotClickAction::Moved, "Moved " + sourceName + "."};
    }
}
