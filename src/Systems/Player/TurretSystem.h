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
        const sf::Texture& projectileTexture; // this is temporary as every spell should just hold a sprite

    public:
        TurretSystem(inventory::Inventory& inventory, sf::RenderWindow& window,
                     const sf::Texture& projectileTexture)
            : inventory(inventory), window(window), projectileTexture(projectileTexture) {}

        void update(const float elapsedTime) override {//this needs serious reimplementation
            struct ShotRequest {
                ecs::Entity turretEntity;
                ecs::Entity owner;
                sf::Vector2f origin;
                sf::Vector2f target;
            };
            std::vector<ShotRequest> shots;

            ecs->view<components::Player, components::Transform, components::Drawable,
                      components::TurretMountLayout>()
                .each([this, elapsedTime, &shots](
                    const ecs::Entity playerEntity,
                    components::Player&,
                    const components::Transform& playerTransform,
                    const components::Drawable& playerDrawable,
                    const components::TurretMountLayout& mountLayout) {
                    if (!playerDrawable.sprite) return;

                    const auto spriteOrigin = playerDrawable.sprite->sprite.getOrigin();
                    const bool firing = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
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
                        turretTransform.setRotation(
                            sf::radians(std::atan2(displacement.x, -displacement.y)));
                        if (firing && turret.readyToShoot()) {
                            shots.push_back({*turretEntity, playerEntity, mountPosition, *target});
                        }
                    }
                });

            for (const auto& shot : shots) {
                if (!ecs->isEntityAlive(shot.turretEntity) ||
                    !ecs->hasComponent<inventory::Turret>(shot.turretEntity)) {
                    continue;
                }
                auto& turret = ecs->getComponent<inventory::Turret>(shot.turretEntity);
                if (turret.readyToShoot() &&
                    turret.shoot(*ecs, shot.owner, shot.origin, shot.target, projectileTexture)) {
                    turret.beginCooldown();
                }
            }
        }
    };
}

#endif //METEORSHIP_TURRETSYSTEM_H
