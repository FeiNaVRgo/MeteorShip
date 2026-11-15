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
        std::unordered_map<ecs::Entity, sf::FloatRect> bounds;
        ecs.view<components::Hitbox, components::BoundingBox, components::Transform>()
            .each([&bounds](
                ecs::Entity entity,
                components::Hitbox& hitbox,
                components::BoundingBox& boundingBox,
                const components::Transform& transform)
            {
                bounds.emplace(entity, boundingBox.bounds);
            });

        std::vector<std::pair<ecs::Entity, ecs::Entity>> candidatePairs; //how much to reserve
        for (const auto& [entity, entityBounds] : bounds) {
            for (const auto candidate : world.getQuadtree().queryPotential(entityBounds)) {
                if (candidate == entity || !getWorld().getECS().hasComponent<components::Hitbox>(candidate)) {
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
            const auto& [rectangles1] = getWorld().getECS().getComponent<components::Hitbox>(first);
            const auto& [rectangles2] = getWorld().getECS().getComponent<components::Hitbox>(second);
            const auto& transform1 = getWorld().getECS().getComponent<components::Transform>(first);
            const auto& transform2 = getWorld().getECS().getComponent<components::Transform>(second);

            bool collision = false;
            for (int i = 0; i < rectangles1.size() && !collision; i++) {
                for (int j = 0; j < rectangles2.size() && !collision; j++) {
                    collision = collision_geometry::checkOBBIntersection2D(
                        collision_geometry::transformOBB(rectangles1[i], transform1),
                        collision_geometry::transformOBB(rectangles2[j], transform2));
                }
            }

            if (collision) {
                getWorld().getEventDispatcher().push(Event::HitDetected{.first = first, .second = second});
            }
        }
    }
}
