#ifndef METEORSHIP_TURRETSYSTEM_H
#define METEORSHIP_TURRETSYSTEM_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>

#include "Components/Drawable.h"
#include "Components/Enemy.h"
#include "Components/Player.h"
#include "Components/TurretMountLayout.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "Inventory/Inventory.h"

namespace ms::systems {
    class TurretSystem : public ecs::System {
        inventory::Inventory& inventory;
        sf::RenderWindow& window;

    public:
        TurretSystem(inventory::Inventory& inventory, sf::RenderWindow& window)
            : inventory(inventory), window(window) {}

        void update(const float elapsedTime) override {//this needs serious reimplementation
            struct ShotRequest {
                ecs::Entity turretEntity;
                ecs::Entity owner;
                sf::Vector2f origin;
                float aimAngleRadians;
            };
            std::vector<ShotRequest> shots;
            const bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
            if (inventory.consumeUiInteraction()) suppressFiringUntilRelease = true;
            if (!mouseDown) suppressFiringUntilRelease = false;
            const bool firing = mouseDown && !suppressFiringUntilRelease;

            ecs->view<components::Player, components::Transform, components::Drawable,
                      components::TurretMountLayout>()
                .each([this, elapsedTime, firing, &shots](
                    const ecs::Entity playerEntity,
                    components::Player&,
                    const components::Transform& playerTransform,
                    const components::Drawable& playerDrawable,
                    const components::TurretMountLayout& mountLayout) {
                    if (!playerDrawable.sprite) return;

                    const auto spriteOrigin = playerDrawable.sprite->sprite.getOrigin();
                    const auto mouseTarget = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                    const auto mountCount = std::min(
                        mountLayout.topLeftOffsets.size(), inventory.turretMountCount());
                    for (std::size_t mountIndex = 0; mountIndex < mountCount; ++mountIndex) {
                        const inventory::SlotAddress address{
                            .collection = inventory::SlotCollection::TurretMount,
                            .index = mountIndex
                        };
                        const auto turretEntity = inventory.entityAt(address);
                        if (!turretEntity || !ecs->isEntityAlive(*turretEntity) ||
                            !ecs->hasComponent<inventory::Turret>(*turretEntity) ||
                            !ecs->hasComponent<components::Transform>(*turretEntity)) {
                            continue;
                        }

                        auto& turret = ecs->getComponent<inventory::Turret>(*turretEntity);
                        auto& turretTransform = ecs->getComponent<components::Transform>(*turretEntity);
                        turret.updateCooldown(elapsedTime);

                        const auto offset = mountLayout.topLeftOffsets[mountIndex] - spriteOrigin;
                        const auto mountPosition = playerTransform.getTransform().transformPoint(offset);
                        turretTransform.setPosition(mountPosition);

                        std::optional<sf::Vector2f> target;
                        if (turret.getTargetMode() == inventory::TurretTargetMode::Manual) {
                            target = mouseTarget;
                        } else {
                            float closestDistanceSquared = std::numeric_limits<float>::max();
                            ecs->view<components::Enemy, components::Transform>().each(//this should be replaced with quadtree quarry
                                [&](const ecs::Entity enemyEntity, components::Enemy&,
                                    const components::Transform& enemyTransform) {
                                    if (enemyEntity == playerEntity) return;
                                    const auto difference = enemyTransform.getPosition() - mountPosition;
                                    const float distanceSquared =
                                        difference.x * difference.x + difference.y * difference.y;
                                    if (distanceSquared < closestDistanceSquared) {
                                        closestDistanceSquared = distanceSquared;
                                        target = enemyTransform.getPosition();
                                    }
                                });
                        }

                        if (!target) continue;
                        const auto displacement = *target - mountPosition;
                        if (displacement.x == 0.f && displacement.y == 0.f) continue;
                        const float aimAngleRadians = std::atan2(displacement.y, displacement.x);
                        turretTransform.setRotation(
                            sf::radians(aimAngleRadians + sf::degrees(90.f).asRadians()));
                        if (firing && turret.readyToShoot()) {
                            shots.push_back({*turretEntity, playerEntity, mountPosition, aimAngleRadians});
                        }
                    }
                });

            for (const auto& shot : shots) {
                if (!ecs->isEntityAlive(shot.turretEntity) ||
                    !ecs->hasComponent<inventory::Turret>(shot.turretEntity)) {
                    continue;
                }
                auto& turret = ecs->getComponent<inventory::Turret>(shot.turretEntity);
                if (turret.readyToShoot()) {
                    turret.shoot(*ecs, shot.owner, shot.origin, shot.aimAngleRadians);
                }
            }
        }

    private:
        bool suppressFiringUntilRelease = false;
    };
}

#endif //METEORSHIP_TURRETSYSTEM_H
