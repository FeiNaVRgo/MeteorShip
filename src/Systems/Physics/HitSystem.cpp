#include "HitSystem.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Components/BoundingBox.h"
#include "Components/Hitbox.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Systems/Physics/CollisionGeometry.h"

namespace ms::systems {
    void HitSystem::update(float) {
        auto& world = getWorld();
        auto& ecs = world.getECS();
        std::unordered_map<ecs::Entity, std::vector<collision_geometry::Polygon>> hitboxes;
        std::unordered_map<ecs::Entity, sf::FloatRect> bounds;
        ecs.view<components::Hitbox, components::BoundingBox, components::Transform>()
            .each([&hitboxes, &bounds](
                const ecs::Entity entity,
                const components::Hitbox& hitbox,
                const components::BoundingBox& boundingBox,
                const components::Transform& transform) {
                auto& entityHitboxes = hitboxes[entity];
                entityHitboxes.reserve(hitbox.rectangles.size());
                for (const auto& rectangle : hitbox.rectangles) {
                    entityHitboxes.push_back(collision_geometry::transformRectangle(
                        rectangle, transform.getTransform()));
                }
                bounds.emplace(entity, boundingBox.bounds);
            });

        std::vector<std::pair<ecs::Entity, ecs::Entity>> candidatePairs;
        for (const auto& [entity, entityBounds] : bounds) {
            for (const auto candidate : world.getQuadtree().queryPotential(entityBounds)) {
                if (candidate == entity || !hitboxes.contains(candidate)) {
                    continue;
                }
                candidatePairs.emplace_back(
                    std::min(entity, candidate), std::max(entity, candidate));
            }
        }
        std::ranges::sort(candidatePairs);
        candidatePairs.erase(
            std::ranges::unique(candidatePairs).begin(), candidatePairs.end());

        for (const auto& [first, second] : candidatePairs) {
            std::optional<collision_geometry::Contact> contact;
            for (const auto& firstBox : hitboxes.at(first)) {
                for (const auto& secondBox : hitboxes.at(second)) {
                    contact = collision_geometry::intersectConvex(firstBox, secondBox);
                    if (contact) {
                        break;
                    }
                }
                if (contact) {
                    break;
                }
            }
            if (contact) {
                world.getEventDispatcher().push(Event::HitDetected{
                    .first = first,
                    .second = second,
                    .normal = contact->normal
                });
            }
        }
    }
}
