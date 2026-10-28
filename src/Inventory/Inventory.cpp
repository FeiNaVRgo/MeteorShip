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

    ui::UINode& Inventory::addTurretModeToggle(
        const std::size_t turretIndex, const sf::Vector2f position) {
        auto& node = uiRoot->add(
            ui::widgets::ButtonWidget{
                .text = "A",
                .onClick = [this, turretIndex] { toggleTurretTargetMode(turretIndex); }
            },
            {UiModeButtonSize, UiModeButtonSize});
        node.alignToParent(ui::Alignment::TopLeft, position);
        turretModeWidgets.push_back({
            turretIndex, node.getWidget<ui::widgets::ButtonWidget>()
        });
        return node;
    }

    void Inventory::buildUI(ui::UINode& root) {
        uiRoot = &root;
        slotWidgets.clear();
        turretModeWidgets.clear();
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
            addTurretModeToggle(
                i,
                {x + UiSlotSize - UiModeButtonSize, slotY + UiSlotSize - UiModeButtonSize});
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

        for (const auto& entry : turretModeWidgets) {
            const auto* turret = turretAt(entry.turretIndex);
            if (!turret) {
                entry.widget->text = "-";
            } else {
                entry.widget->text = turret->getTargetMode() == TurretTargetMode::Automatic ? "A" : "M";
            }
        }
    }

    void Inventory::handleSlotClick(const SlotAddress& address) {
        const auto result = clickSlot(address);
        if (statusWidget) statusWidget->text = result.message;
    }

    std::optional<ecs::Entity>* Inventory::spellSlot(const SlotAddress& address) {
        switch (address.collection) {
            case SlotCollection::SpellInventory:
                return address.index < spells.size() ? &spells[address.index] : nullptr;
            case SlotCollection::TurretSpell:
                if (address.turretIndex >= turrets.size() || !turrets[address.turretIndex] ||
                    !entityManager || !entityManager->hasComponent<Turret>(*turrets[address.turretIndex]) ||
                    address.index >= entityManager->getComponent<Turret>(
                        *turrets[address.turretIndex]).spellOrder.size()) {
                    return nullptr;
                }
                return &entityManager->getComponent<Turret>(
                    *turrets[address.turretIndex]).spellOrder[address.index];
            case SlotCollection::TurretMount:
                return nullptr;
        }
        return nullptr;
    }

    std::optional<ecs::Entity>* Inventory::turretSlot(const SlotAddress& address) {
        if (address.collection != SlotCollection::TurretMount || address.index >= turrets.size()) {
            return nullptr;
        }
        return &turrets[address.index];
    }

    const std::optional<ecs::Entity>* Inventory::spellSlot(const SlotAddress& address) const {
        switch (address.collection) {
            case SlotCollection::SpellInventory:
                return address.index < spells.size() ? &spells[address.index] : nullptr;
            case SlotCollection::TurretSpell:
                if (address.turretIndex >= turrets.size() || !turrets[address.turretIndex] ||
                    !entityManager || !entityManager->hasComponent<Turret>(*turrets[address.turretIndex]) ||
                    address.index >= entityManager->getComponent<Turret>(
                        *turrets[address.turretIndex]).spellOrder.size()) {
                    return nullptr;
                }
                return &entityManager->getComponent<Turret>(
                    *turrets[address.turretIndex]).spellOrder[address.index];
            case SlotCollection::TurretMount:
                return nullptr;
        }
        return nullptr;
    }

    const std::optional<ecs::Entity>* Inventory::turretSlot(const SlotAddress& address) const {
        if (address.collection != SlotCollection::TurretMount || address.index >= turrets.size()) {
            return nullptr;
        }
        return &turrets[address.index];
    }

    std::size_t Inventory::turretSpellSlotCount(const std::size_t turretIndex) const {
        if (turretIndex >= turrets.size() || !turrets[turretIndex] || !entityManager ||
            !entityManager->hasComponent<Turret>(*turrets[turretIndex])) {
            return 0;
        }
        return entityManager->getComponent<Turret>(*turrets[turretIndex]).spellSlotCount();
    }

    void Inventory::bindPlayerEntity(const ecs::Entity player) {
        if (!entityManager || !entityManager->isEntityAlive(player)) return;
        playerEntity = player;
        for (const auto& turret : turrets) {
            if (turret) prepareTurretEntity(*turret);
        }
    }

    void Inventory::prepareTurretEntity(const ecs::Entity entity) {
        if (!entityManager || !entityManager->isEntityAlive(entity) ||
            !entityManager->hasComponent<Turret>(entity)) return;
        if (!entityManager->hasComponent<components::Transform>(entity)) {
            entityManager->addComponent<components::Transform>(entity);
        }

        auto* icon = entityManager->getComponent<Turret>(entity).getIcon();
        if (icon && !entityManager->hasComponent<components::Drawable>(entity)) {
            const auto& texture = icon->getTexture();
            const auto rect = icon->getTextureRect();
            utility::AnimatedSprite sprite(texture, rect, 1, std::nullopt);
            sprite.sprite.setColor(icon->getColor());
            components::Drawable drawable;
            drawable.sprite = std::move(sprite);
            entityManager->addComponent<components::Drawable>(entity, std::move(drawable));
        }
    }

    const Turret* Inventory::turretAt(const std::size_t turretIndex) const {
        if (turretIndex >= turrets.size() || !turrets[turretIndex] || !entityManager ||
            !entityManager->isEntityAlive(*turrets[turretIndex]) ||
            !entityManager->hasComponent<Turret>(*turrets[turretIndex])) return nullptr;
        return &entityManager->getComponent<Turret>(*turrets[turretIndex]);
    }

    bool Inventory::setSpell(const SlotAddress& address, Spell spell) {
        auto* slot = spellSlot(address);
        if (!slot || !entityManager) return false;
        const auto entity = entityManager->createEntity();
        entityManager->addComponent<Spell>(entity, std::move(spell));
        if (*slot) destroyStoredEntity(**slot);
        *slot = entity;
        refreshUI();
        return true;
    }

    bool Inventory::setTurret(const std::size_t turretIndex, Turret turret) {
        if (turretIndex >= turrets.size() || !entityManager) return false;
        const auto entity = entityManager->createEntity();
        entityManager->addComponent<Turret>(entity, std::move(turret));
        if (turrets[turretIndex]) destroyStoredEntity(*turrets[turretIndex]);
        turrets[turretIndex] = entity;
        if (playerEntity) prepareTurretEntity(entity);
        refreshUI();
        return true;
    }

    bool Inventory::toggleTurretTargetMode(const std::size_t turretIndex) {
        if (turretIndex >= turrets.size() || !turrets[turretIndex] || !entityManager ||
            !entityManager->isEntityAlive(*turrets[turretIndex]) ||
            !entityManager->hasComponent<Turret>(*turrets[turretIndex])) {
            return false;
        }
        entityManager->getComponent<Turret>(*turrets[turretIndex]).toggleTargetMode();
        refreshUI();
        return true;
    }

    std::optional<SlotAddress> Inventory::storeSpellEntity(const ecs::Entity entity) {
        if (!entityManager || !entityManager->isEntityAlive(entity) ||
            !entityManager->hasComponent<Spell>(entity) || containsEntity(entity)) {
            return std::nullopt;
        }
        for (std::size_t i = 0; i < spells.size(); ++i) {
            if (spells[i]) continue;
            spells[i] = entity;
            refreshUI();
            return SlotAddress{SlotCollection::SpellInventory, i};
        }
        return std::nullopt;
    }

    std::optional<SlotAddress> Inventory::storeTurretEntity(const ecs::Entity entity) {
        if (!entityManager || !entityManager->isEntityAlive(entity) ||
            !entityManager->hasComponent<Turret>(entity) || containsEntity(entity)) {
            return std::nullopt;
        }
        for (std::size_t i = 0; i < turrets.size(); ++i) {
            if (turrets[i]) continue;
            turrets[i] = entity;
            if (playerEntity) prepareTurretEntity(entity);
            refreshUI();
            return SlotAddress{SlotCollection::TurretMount, i};
        }
        return std::nullopt;
    }

    std::optional<SlotAddress> Inventory::storeSpell(Spell spell) {
        if (!entityManager) return std::nullopt;
        const auto entity = entityManager->createEntity();
        entityManager->addComponent<Spell>(entity, std::move(spell));
        if (const auto slot = storeSpellEntity(entity)) return slot;
        entityManager->destroyEntity(entity);
        return std::nullopt;
    }

    std::optional<SlotAddress> Inventory::storeTurret(Turret turret) {
        if (!entityManager) return std::nullopt;
        const auto entity = entityManager->createEntity();
        entityManager->addComponent<Turret>(entity, std::move(turret));
        if (const auto slot = storeTurretEntity(entity)) return slot;
        entityManager->destroyEntity(entity);
        return std::nullopt;
    }

    std::optional<ecs::Entity> Inventory::takeSpell(const SlotAddress& address) {
        if (address.collection == SlotCollection::TurretMount) return std::nullopt;
        if (!hasItem(address)) return std::nullopt;
        auto* slot = spellSlot(address);
        if (!slot || !*slot) return std::nullopt;
        const auto removed = **slot;
        slot->reset();
        if (selectedSlot && *selectedSlot == address) selectedSlot.reset();
        refreshUI();
        return removed;
    }

    std::optional<ecs::Entity> Inventory::takeTurret(const std::size_t turretIndex) {
        if (!hasItem({SlotCollection::TurretMount, turretIndex})) return std::nullopt;
        const auto removed = *turrets[turretIndex];
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

    bool Inventory::containsEntity(const ecs::Entity entity) const {
        for (const auto& spell : spells) {
            if (spell && *spell == entity) return true;
        }
        for (const auto& turretEntity : turrets) {
            if (turretEntity && *turretEntity == entity) return true;
            if (!turretEntity || !entityManager || !entityManager->isEntityAlive(*turretEntity) ||
                !entityManager->hasComponent<Turret>(*turretEntity)) {
                continue;
            }
            const auto& turret = entityManager->getComponent<Turret>(*turretEntity);
            for (const auto& spell : turret.spellOrder) {
                if (spell && *spell == entity) return true;
            }
        }
        return false;
    }

    void Inventory::destroyStoredEntity(const ecs::Entity entity) {
        if (!entityManager || !entityManager->isEntityAlive(entity)) return;
        if (entityManager->hasComponent<Turret>(entity)) {
            const auto spellEntities = entityManager->getComponent<Turret>(entity).spellOrder;
            for (const auto& spell : spellEntities) {
                if (spell && entityManager->isEntityAlive(*spell) &&
                    entityManager->hasComponent<Spell>(*spell)) {
                    entityManager->destroyEntity(*spell);
                }
            }
        }
        entityManager->destroyEntity(entity);
    }

    std::optional<std::size_t> Inventory::addTurretSpellSlot(const std::size_t turretIndex) {
        if (turretIndex >= turrets.size() || !turrets[turretIndex] || !entityManager ||
            !entityManager->isEntityAlive(*turrets[turretIndex]) ||
            !entityManager->hasComponent<Turret>(*turrets[turretIndex])) return std::nullopt;
        const auto addedSlot = entityManager->getComponent<Turret>(
            *turrets[turretIndex]).addSpellSlot();
        refreshUI();
        return addedSlot;
    }

    bool Inventory::removeTurretSpellSlot(const std::size_t turretIndex, const std::size_t slotIndex) {
        auto* turret = turretIndex < turrets.size() && turrets[turretIndex] && entityManager &&
            entityManager->isEntityAlive(*turrets[turretIndex]) &&
            entityManager->hasComponent<Turret>(*turrets[turretIndex])
                ? &entityManager->getComponent<Turret>(*turrets[turretIndex])
                : nullptr;
        if (!turret || !turret->removeSpellSlot(slotIndex)) return false;
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
        auto* turret = turretIndex < turrets.size() && turrets[turretIndex] && entityManager &&
            entityManager->isEntityAlive(*turrets[turretIndex]) &&
            entityManager->hasComponent<Turret>(*turrets[turretIndex])
                ? &entityManager->getComponent<Turret>(*turrets[turretIndex])
                : nullptr;
        if (!turret) return false;
        const auto oldCount = turret->spellSlotCount();
        if (!turret->resizeSpellSlots(count)) return false;
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
            if (!sourceSlot || !destinationSlot || !*sourceSlot ||
                !entityManager || !entityManager->isEntityAlive(**sourceSlot) ||
                !entityManager->hasComponent<Spell>(**sourceSlot)) return false;
            sourceSlot->swap(*destinationSlot);
            return true;
        }

        auto* sourceSlot = turretSlot(source);
        auto* destinationSlot = turretSlot(destination);
        if (!sourceSlot || !destinationSlot || !*sourceSlot ||
            !entityManager || !entityManager->isEntityAlive(**sourceSlot) ||
            !entityManager->hasComponent<Turret>(**sourceSlot)) return false;
        sourceSlot->swap(*destinationSlot);
        return true;
    }

    bool Inventory::clear(const SlotAddress& address) {
        auto* slot = address.collection == SlotCollection::TurretMount
            ? turretSlot(address)
            : spellSlot(address);
        if (!slot || !*slot) return false;
        destroyStoredEntity(**slot);
        slot->reset();
        if (selectedSlot && *selectedSlot == address) selectedSlot.reset();
        refreshUI();
        return true;
    }

    bool Inventory::hasItem(const SlotAddress& address) const {
        if (!entityManager) return false;
        if (const auto* slot = spellSlot(address); slot && *slot) {
            return entityManager->isEntityAlive(**slot) &&
                entityManager->hasComponent<Spell>(**slot);
        }
        if (const auto* slot = turretSlot(address); slot && *slot) {
            return entityManager->isEntityAlive(**slot) &&
                entityManager->hasComponent<Turret>(**slot);
        }
        return false;
    }

    std::optional<ecs::Entity> Inventory::entityAt(const SlotAddress& address) const {
        if (!hasItem(address)) return std::nullopt;
        if (address.collection == SlotCollection::TurretMount) {
            return turretSlot(address)->value();
        }
        return spellSlot(address)->value();
    }

    std::optional<std::string> Inventory::itemName(const SlotAddress& address) const {
        const auto entity = entityAt(address);
        if (!entity) return std::nullopt;
        if (address.collection == SlotCollection::TurretMount) {
            return entityManager->getComponent<Turret>(*entity).getName();
        }
        return entityManager->getComponent<Spell>(*entity).getName();
    }

    const sf::Sprite* Inventory::itemIcon(const SlotAddress& address) const {
        const auto entity = entityAt(address);
        if (!entity) return nullptr;
        return address.collection == SlotCollection::TurretMount
            ? entityManager->getComponent<Turret>(*entity).getIcon()
            : entityManager->getComponent<Spell>(*entity).getIcon();
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
