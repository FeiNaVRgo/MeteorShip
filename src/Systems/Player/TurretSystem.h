#ifndef METEORSHIP_TURRETSYSTEM_H
#define METEORSHIP_TURRETSYSTEM_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_set>
#include <utility>
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
#include "Systems/SpatialHashing/Quadtree.h"
#include "Utility/UtiFuncs.h"

namespace ms::systems {
    class TurretSystem : public ecs::System {
        struct ShotRequest {
            std::size_t mountIndex;
            ecs::Entity owner;
            sf::Vector2f origin;
            float aimAngleRadians;
        };

        inventory::Inventory& inventory;
        sf::RenderWindow& window;
        Quadtree& quadtree;

    public:
        TurretSystem(inventory::Inventory& inventory, sf::RenderWindow& window,
                     Quadtree& quadtree)
            : inventory(inventory), window(window), quadtree(quadtree) {}

        void update(const float elapsedTime) override {
            synchronizeTurretEntities();
            std::vector<ShotRequest> shots;
            const bool firing = isFiring();
            const auto mouseTarget = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            ecs->view<components::Player, components::Transform, components::Drawable,
                      components::TurretMountLayout>()
                .each([this, elapsedTime, firing, mouseTarget, &shots](
                    const ecs::Entity playerEntity,
                    components::Player&,
                    const components::Transform& playerTransform,
                    const components::Drawable& playerDrawable,
                    const components::TurretMountLayout& mountLayout) {
                    if (!playerDrawable.sprite) return;
                    updatePlayerTurrets(playerEntity, playerTransform, playerDrawable,
                                        mountLayout, mouseTarget, elapsedTime, firing, shots);
                });
            fireShots(shots);
        }

    private:
        bool suppressFiringUntilRelease = false;
        std::vector<std::optional<ecs::Entity>> turretEntities;

        bool isFiring() {
            const bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
            suppressFiringUntilRelease |= inventory.consumeUiInteraction();
            if (!mouseDown) suppressFiringUntilRelease = false;
            return mouseDown && !suppressFiringUntilRelease;
        }

        void synchronizeTurretEntities() {
            const auto mountCount = inventory.turretMountCount();
            while (turretEntities.size() > mountCount) {
                if (turretEntities.back() && ecs->isEntityAlive(*turretEntities.back())) {
                    ecs->destroyEntity(*turretEntities.back());
                }
                turretEntities.pop_back();
            }
            turretEntities.resize(mountCount);

            for (std::size_t index = 0; index < mountCount; ++index) {
                const auto* turret = inventory.turretAt(index);
                auto& entity = turretEntities[index];
                if (!turret) {
                    if (entity && ecs->isEntityAlive(*entity)) ecs->destroyEntity(*entity);
                    entity.reset();
                    continue;
                }

                if (!entity || !ecs->isEntityAlive(*entity)) {
                    entity = ecs->createEntity();
                    ecs->addComponent<components::Transform>(*entity);
                }

                const auto* icon = turret->getIcon();
                if (icon) {
                    if (ecs->hasComponent<components::Drawable>(*entity)) {
                        auto& drawable = ecs->getComponent<components::Drawable>(*entity);
                        if (drawable.sprite) {
                            drawable.sprite->sprite = *icon;
                        } else {
                            drawable.sprite.emplace(
                                icon->getTexture(), icon->getTextureRect(), 1, std::nullopt);
                        }
                        drawable.sprite->sprite.setColor(icon->getColor());
                    } else {
                        utility::AnimatedSprite sprite(
                            icon->getTexture(), icon->getTextureRect(), 1, std::nullopt);
                        sprite.sprite.setColor(icon->getColor());
                        components::Drawable drawable;
                        drawable.sprite = std::move(sprite);
                        ecs->addComponent<components::Drawable>(*entity, std::move(drawable));
                    }
                } else if (ecs->hasComponent<components::Drawable>(*entity)) {
                    ecs->removeComponent<components::Drawable>(*entity);
                }
            }
        }

        void updatePlayerTurrets(
            const ecs::Entity playerEntity,
            const components::Transform& playerTransform,
            const components::Drawable& playerDrawable,
            const components::TurretMountLayout& mountLayout,
            const sf::Vector2f mouseTarget,
            const float elapsedTime,
            const bool firing,
            std::vector<ShotRequest>& shots) {
            const auto mountCount = std::min(
                mountLayout.topLeftOffsets.size(), inventory.turretMountCount());
            const auto spriteOrigin = playerDrawable.sprite->sprite.getOrigin();
            for (std::size_t index = 0; index < mountCount; ++index) {
                if (auto shot = updateTurret(
                        playerEntity, playerTransform, spriteOrigin,
                        mountLayout.topLeftOffsets[index], index, mouseTarget,
                        elapsedTime, firing)) {
                    shots.push_back(*shot);
                }
            }
        }

        std::optional<ShotRequest> updateTurret(
            const ecs::Entity playerEntity,
            const components::Transform& playerTransform,
            const sf::Vector2f spriteOrigin,
            const sf::Vector2f mountOffset,
            const std::size_t mountIndex,
            const sf::Vector2f mouseTarget,
            const float elapsedTime,
            const bool firing) {
            auto* turret = inventory.turretAt(mountIndex);
            if (!turret || mountIndex >= turretEntities.size() ||
                !turretEntities[mountIndex] ||
                !ecs->isEntityAlive(*turretEntities[mountIndex])) {
                return std::nullopt;
            }

            auto& turretTransform = ecs->getComponent<components::Transform>(*turretEntities[mountIndex]);
            turret->updateCooldown(elapsedTime);

            const auto mountPosition = playerTransform.getTransform().transformPoint(
                mountOffset - spriteOrigin);
            turretTransform.setPosition(mountPosition);

            const auto target = turret->getTargetMode() == inventory::TurretTargetMode::Manual
                ? std::optional{mouseTarget}
                : findNearestEnemy(playerEntity, mountPosition);
            if (!target) return std::nullopt;

            const auto displacement = *target - mountPosition;
            if (displacement.x == 0.f && displacement.y == 0.f) return std::nullopt;

            const float aimAngle = utility::angleFromDirection(displacement).asRadians();
            turretTransform.setRotation(utility::rotationFromDirection(displacement));
            if (!firing || !turret->readyToShoot()) return std::nullopt;

            return ShotRequest{
                .mountIndex = mountIndex,
                .owner = playerEntity,
                .origin = mountPosition,
                .aimAngleRadians = aimAngle
            };
        }

        void fireShots(const std::vector<ShotRequest>& shots) {
            for (const auto& [mountIndex, owner, origin, aimAngleRadians] : shots) {
                if (auto* turret = inventory.turretAt(mountIndex)) {
                    turret->shoot(*ecs, owner, origin, aimAngleRadians);
                }
            }
        }

        std::optional<sf::Vector2f> findNearestEnemy(
            const ecs::Entity playerEntity, const sf::Vector2f origin) const {
            const auto& bounds = quadtree.worldBounds;
            const float maxRadius = std::max({
                std::abs(origin.x - bounds.position.x),
                std::abs(origin.x - (bounds.position.x + bounds.size.x)),
                std::abs(origin.y - bounds.position.y),
                std::abs(origin.y - (bounds.position.y + bounds.size.y))
            });
            float radius = std::min(64.f, maxRadius);
            std::optional<sf::Vector2f> closest;
            float closestDistanceSquared = std::numeric_limits<float>::max();
            std::unordered_set<ecs::Entity> checked;

            while (true) {
                const sf::FloatRect searchBounds(
                    origin - sf::Vector2f(radius, radius),
                    sf::Vector2f(radius * 2.f, radius * 2.f));
                for (const auto entity : quadtree.query(searchBounds)) {
                    if (checked.insert(entity).second) {
                        considerEnemy(entity, playerEntity, origin, closest, closestDistanceSquared);
                    }
                }

                if (radius >= maxRadius ||
                    (closest && closestDistanceSquared <= radius * radius)) {
                    return closest;
                }
                radius = std::min(maxRadius, radius * 2.f);
            }
        }

        void considerEnemy(
            const ecs::Entity entity,
            const ecs::Entity playerEntity,
            const sf::Vector2f origin,
            std::optional<sf::Vector2f>& closest,
            float& closestDistanceSquared) const {
            if (entity == playerEntity || !ecs->isEntityAlive(entity) ||
                !ecs->hasComponent<components::Enemy>(entity) ||
                !ecs->hasComponent<components::Transform>(entity)) {
                return;
            }

            const auto position = ecs->getComponent<components::Transform>(entity).getPosition();
            const float distanceSquared = utility::squaredDistance(position, origin);
            if (distanceSquared < closestDistanceSquared) {
                closestDistanceSquared = distanceSquared;
                closest = position;
            }
        }
    };
}

#endif //METEORSHIP_TURRETSYSTEM_H
