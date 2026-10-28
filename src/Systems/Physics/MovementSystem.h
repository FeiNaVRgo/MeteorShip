//
// Created by iformat on 10/13/26.
//

#ifndef METEORSHIP_MOVEMENTSYSTEM_H
#define METEORSHIP_MOVEMENTSYSTEM_H

#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"

namespace ms::systems {
    class MovementSystem : public ecs::System {
    public:
        void update(float deltaTime) override {
            ecs->view<components::Rigidbody, components::Transform>()
                .each(
                    [deltaTime](ecs::Entity const& entity,
                                components::Rigidbody& rigidbody,
                                components::Transform& transform)
                    {
                        rigidbody.velocity += rigidbody.acceleration * deltaTime;
                        transform.move(rigidbody.velocity * deltaTime);
                        
                        rigidbody.bounds.position = transform.getPosition() - rigidbody.bounds.size / 2.f;
                    }
            );
        }
    };
}

#endif //METEORSHIP_MOVEMENTSYSTEM_H
