//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_SPATIALHASH_H
#define METEORSHIP_SPATIALHASH_H

#include "Quadtree.h"
#include "ECS/ECS.h"
#include "Components/Rigidbody.h"
#include "Events/EventDispatcher.h"

namespace ms::systems {
    class SpatialHash : public ecs::System {
    private:
        Quadtree& m_quadtree;
        EventDispatcher& eventDispatcher;
    public:
        explicit SpatialHash(Quadtree& quadtree, EventDispatcher& eventDispatcher) : m_quadtree(quadtree), eventDispatcher(eventDispatcher) {}

        void update(float deltaTime) override {
            ecs->view<components::Rigidbody>()
            .each([this](ecs::Entity const& entity, components::Rigidbody& rigidbody) {
                m_quadtree.update(entity, rigidbody.bounds);
                if (!m_quadtree.worldBounds.findIntersection(rigidbody.bounds).has_value()) {
                    eventDispatcher.push(Event(Event::RemoveEntity(entity)));
                }
            });
        }
    };
}

#endif //METEORSHIP_SPATIALHASH_H
