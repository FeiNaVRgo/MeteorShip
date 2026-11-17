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
#include "ECS/World.h"
#include "Inventory/Inventory.h"
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

    public:
        explicit TurretSystem(inventory::Inventory& inventory)
            : inventory(inventory) {}

        void update(const float elapsedTime) override {
            synchronizeTurretEntities();
            std::vector<ShotRequest> shots;
            const bool firing = isFiring();
            auto& window = getWorld().getMainWindow();
            const auto mouseTarget = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            getWorld().getECS()
                .view<components::Player, components::Transform, components::Drawable,
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
            inventory.updateUI();
        }

    private:
        bool suppressFiringUntilRelease = false;
        std::vector<std::optional<ecs::Entity>> turretEntities;

        bool isFiring() {
            const bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
            auto& window = getWorld().getMainWindow();
            const auto mousePosition = sf::Mouse::getPosition(window);
            if (mouseDown && inventory.isPointerOverInteractiveWidget({
                    static_cast<float>(mousePosition.x), static_cast<float>(mousePosition.y)})) {
                suppressFiringUntilRelease = true;
            }
            suppressFiringUntilRelease |= inventory.consumeUiInteraction();
            if (!mouseDown) suppressFiringUntilRelease = false;
            return mouseDown && !suppressFiringUntilRelease;
        }

        void synchronizeTurretEntities() {
            auto& entityManager = getWorld().getECS();
            const auto mountCount = inventory.turretMountCount();
            while (turretEntities.size() > mountCount) {
                if (turretEntities.back() && entityManager.isEntityAlive(*turretEntities.back())) {
                    entityManager.destroyEntity(*turretEntities.back());
                }
                turretEntities.pop_back();
            }
            turretEntities.resize(mountCount);

            for (std::size_t index = 0; index < mountCount; ++index) {
                const auto* turret = inventory.turretAt(index);
                auto& entity = turretEntities[index];
                if (!turret) {
                    if (entity && entityManager.isEntityAlive(*entity)) {
                        entityManager.destroyEntity(*entity);
                    }
                    entity.reset();
                    continue;
                }

                if (!entity || !entityManager.isEntityAlive(*entity)) {
                    entity = entityManager.createEntity();
                    entityManager.addComponent<components::Transform>(*entity);
                }

                const auto* icon = turret->getIcon();
                if (icon) {
                    if (entityManager.hasComponent<components::Drawable>(*entity)) {
                        auto& drawable = entityManager.getComponent<components::Drawable>(*entity);
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
                        entityManager.addComponent<components::Drawable>(*entity, std::move(drawable));
                    }
                } else if (entityManager.hasComponent<components::Drawable>(*entity)) {
                    entityManager.removeComponent<components::Drawable>(*entity);
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
            auto& entityManager = getWorld().getECS();
            auto* turret = inventory.turretAt(mountIndex);
            if (!turret || mountIndex >= turretEntities.size() ||
                !turretEntities[mountIndex] ||
                !entityManager.isEntityAlive(*turretEntities[mountIndex])) {
                return std::nullopt;
            }

            auto& turretTransform =
                entityManager.getComponent<components::Transform>(*turretEntities[mountIndex]);
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
            auto& world = getWorld();
            for (const auto& [mountIndex, owner, origin, aimAngleRadians] : shots) {
                if (auto* turret = inventory.turretAt(mountIndex)) {
                    turret->shoot(world, owner, origin, aimAngleRadians);
                }
            }
        }

        std::optional<sf::Vector2f> findNearestEnemy(
            const ecs::Entity playerEntity, const sf::Vector2f origin) const {
            const auto& quadtree = getWorld().getQuadtree();
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
            auto& entityManager = getWorld().getECS();
            if (entity == playerEntity || !entityManager.isEntityAlive(entity) ||
                !entityManager.hasComponent<components::Enemy>(entity) ||
                !entityManager.hasComponent<components::Transform>(entity)) {
                return;
            }

            const auto position =
                entityManager.getComponent<components::Transform>(entity).getPosition();
            const float distanceSquared = utility::squaredDistance(position, origin);
            if (distanceSquared < closestDistanceSquared) {
                closestDistanceSquared = distanceSquared;
                closest = position;
            }
        }
    };
}

#endif //METEORSHIP_TURRETSYSTEM_H
