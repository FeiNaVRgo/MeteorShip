//
// Created by iformat on 10/24/26.
//

#ifndef METEORSHIP_BTCONTEXT_H
#define METEORSHIP_BTCONTEXT_H
#include <memory>
#include <optional>

#include "ECS/World.h"

namespace ms::bt_nodes {
    struct BTContext {
        std::weak_ptr<World> world;
        ecs::Entity entity;
        std::optional<ecs::Entity> playerEntity;
        float deltaTime = 0.f;
    };
}

#endif //METEORSHIP_BTCONTEXT_H