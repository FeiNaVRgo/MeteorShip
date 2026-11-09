//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_SPATIALHASH_H
#define METEORSHIP_SPATIALHASH_H

#include <unordered_set>

#include "Quadtree.h"
#include "ECS/ECS.h"
#include "ECS/World.h"
#include "Components/Rigidbody.h"
#include "Events/EventDispatcher.h"

namespace ms::systems {
    class SpatialHash : public ecs::System {
    public:
        void update(float deltaTime) override {
            auto& world = getWorld();
            auto& entityManager = world.getECS();
            std::unordered_set<ecs::Entity> currentEntities;
            entityManager.view<components::Rigidbody>().each(
                [&currentEntities](const ecs::Entity entity, components::Rigidbody&) {
                    currentEntities.insert(entity);
                });

            auto& quadtree = world.getQuadtree();
            std::unordered_set<ecs::Entity> indexedEntities;
            for (const auto entity : quadtree.queryAll()) {
                if (indexedEntities.insert(entity).second && !currentEntities.contains(entity)) {
                    quadtree.remove(entity);
                }
            }

            entityManager.view<components::Rigidbody>()
            .each([this](ecs::Entity const& entity, components::Rigidbody& rigidbody) {
                auto& world = getWorld();
                world.getQuadtree().update(entity, rigidbody.bounds);
                if (!world.getQuadtree().worldBounds.findIntersection(rigidbody.bounds).has_value()) {
                    world.getEventDispatcher().push(Event(Event::RemoveEntity(entity)));
                }
            });
        }
    };
}

#endif //METEORSHIP_SPATIALHASH_H
