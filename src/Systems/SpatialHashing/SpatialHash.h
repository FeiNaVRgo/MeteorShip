//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_SPATIALHASH_H
#define METEORSHIP_SPATIALHASH_H

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <vector>

#include "Quadtree.h"
#include "ECS/ECS.h"
#include "ECS/World.h"
#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Hitbox.h"
#include "Components/Transform.h"
#include "Events/EventDispatcher.h"

namespace ms::systems {
    class SpatialHash : public ecs::System {
    public:
        void update(float deltaTime) override {
            auto& world = getWorld();
            auto& entityManager = world.getECS();
            std::unordered_set<ecs::Entity> currentEntities;
            entityManager.view<components::BoundingBox>().each(
                [&currentEntities](const ecs::Entity entity, components::BoundingBox&) {
                    currentEntities.insert(entity);
                });

            auto& quadtree = world.getQuadtree();
            std::unordered_set<ecs::Entity> indexedEntities;
            for (const auto entity : quadtree.queryAll()) {
                if (indexedEntities.insert(entity).second && !currentEntities.contains(entity)) {
                    quadtree.remove(entity);
                }
            }

            entityManager.view<components::BoundingBox, components::Transform>().each(
                [this](const ecs::Entity entity,
                       components::BoundingBox& boundingBox,
                       const components::Transform& transform) {
                    auto& entityManager = getWorld().getECS();
                    std::vector<sf::Vector2f> points;
                    const auto transformPoint = [&transform, &points](const sf::Vector2f point) {
                        points.push_back(transform.getTransform().transformPoint(point));
                    };

                    if (entityManager.hasComponent<components::CollisionShape>(entity)) {
                        for (const auto point :
                             entityManager.getComponent<components::CollisionShape>(entity).vertices) {
                            transformPoint(point);
                        }
                    }
                    if (entityManager.hasComponent<components::Hitbox>(entity)) {
                        const auto& hitbox =
                            entityManager.getComponent<components::Hitbox>(entity);
                        for (const auto& rectangle : hitbox.rectangles) {
                            const auto left = rectangle.position.x;
                            const auto top = rectangle.position.y;
                            const auto right = left + rectangle.size.x;
                            const auto bottom = top + rectangle.size.y;
                            transformPoint({left, top});
                            transformPoint({right, top});
                            transformPoint({right, bottom});
                            transformPoint({left, bottom});
                        }
                    }
                    if (points.empty()) {
                        const auto halfSize = boundingBox.bounds.size / 2.f;
                        transformPoint({-halfSize.x, -halfSize.y});
                        transformPoint({halfSize.x, -halfSize.y});
                        transformPoint({halfSize.x, halfSize.y});
                        transformPoint({-halfSize.x, halfSize.y});
                    }

                    sf::Vector2f minimum(
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max());
                    sf::Vector2f maximum(
                        std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest());
                    for (const auto point : points) {
                        minimum.x = std::min(minimum.x, point.x);
                        minimum.y = std::min(minimum.y, point.y);
                        maximum.x = std::max(maximum.x, point.x);
                        maximum.y = std::max(maximum.y, point.y);
                    }
                    boundingBox.bounds = {minimum, maximum - minimum};

                    auto& world = getWorld();
                    world.getQuadtree().update(entity, boundingBox.bounds);
                    if (!world.getQuadtree().worldBounds.findIntersection(
                            boundingBox.bounds)) {
                        world.getEventDispatcher().push(Event(Event::RemoveEntity(entity)));
                    }
                });
        }
    };
}

#endif //METEORSHIP_SPATIALHASH_H
