//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_SPATIALHASH_H
#define METEORSHIP_SPATIALHASH_H

#include "Quadtree.h"
#include "ECS/ECS.h"
#include "Components/Rigidbody.h"

namespace ms::systems {
    class SpatialHash : public ms::ecs::System {
    private:
        Quadtree& m_quadtree;
    public:
        void setEcs(ecs::ECS *_ecs) override {
            ecs = _ecs;
        }

        void update(float deltaTime) override {
            ecs->view<components::Rigidbody>().each([&](ecs::Entity& entity, components::Rigidbody& rigidbody) {

            });
        }
    };
}

#endif //METEORSHIP_SPATIALHASH_H
