#include "CollisionSystem.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>

#include "Components/Rigidbody.h"
#include "Systems/Physics/OpenCLCollisionDetector.h"

namespace ms::systems {
    namespace {
        using Aabb = std::array<float, 4>;

        bool intersects(const Aabb &a, const Aabb &b) {
            return a[0] < b[2] && a[2] > b[0] &&
                   a[1] < b[3] && a[3] > b[1];
        }

        Aabb toAabb(const sf::FloatRect &bounds) {
            return {
                bounds.position.x,
                bounds.position.y,
                bounds.position.x + bounds.size.x,
                bounds.position.y + bounds.size.y
            };
        }
    }

    CollisionSystem::CollisionSystem(
        Quadtree &quadtree, EventDispatcher &eventDispatcher,
        OpenCLCollisionDetector &collisionDetector)
        : quadtree(quadtree),
          eventDispatcher(eventDispatcher),
          collisionDetector(collisionDetector) {
    }

    void CollisionSystem::update(float) {
        std::unordered_map<ecs::Entity, Aabb> entityBounds;
        ecs->view<components::Rigidbody>().each(
            [&entityBounds](ecs::Entity entity, const components::Rigidbody &rigidbody) {
                entityBounds.emplace(entity, toAabb(rigidbody.bounds));
            });

        std::vector<std::pair<ecs::Entity, ecs::Entity> > candidatePairs;
        for (const auto &[entity, bounds]: entityBounds) {
            const sf::FloatRect queryBounds(
                {bounds[0], bounds[1]}, {bounds[2] - bounds[0], bounds[3] - bounds[1]});
            for (const auto candidate: quadtree.queryPotential(queryBounds)) {
                if (candidate == entity || !entityBounds.contains(candidate)) {
                    continue;
                }
                candidatePairs.emplace_back(
                    std::min(entity, candidate), std::max(entity, candidate));
            }
        }
        std::ranges::sort(candidatePairs);
        candidatePairs.erase(std::ranges::unique(candidatePairs).begin(),
                             candidatePairs.end());

        std::vector<Aabb> boxes;
        boxes.reserve(candidatePairs.size() * 2);
        for (const auto &[first, second]: candidatePairs) {
            boxes.push_back(entityBounds.at(first));
            boxes.push_back(entityBounds.at(second));
        }

        const auto gpuOverlaps = collisionDetector.detect(boxes, candidatePairs.size());
        collisions.clear();
        for (size_t i = 0; i < candidatePairs.size(); ++i) {
            const bool collision = gpuOverlaps
                                       ? (*gpuOverlaps)[i] != 0
                                       : intersects(boxes[i * 2], boxes[i * 2 + 1]);
            if (collision) {
                collisions.push_back(candidatePairs[i]);
                eventDispatcher.push(Event::CollisionDetected{
                    .first = candidatePairs[i].first, .second = candidatePairs[i].second
                });
            }
        }
    }
}
