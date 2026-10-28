//
// Created by ricardo on 17.10.2026.
//

#ifndef METEORSHIP_WORLD_H
#define METEORSHIP_WORLD_H

#include <memory>
#include <utility>
#include "ECS.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"

namespace ms {
    class Game;

    class World {
        ecs::ECS ecs;
        systems::Quadtree m_quadtree;
        EventDispatcher eventDispatcher;
    public:
        World(Game& game);

        ecs::ECS& getECS() { return ecs; }
        systems::Quadtree& getQuadtree() { return m_quadtree; }
        EventDispatcher& getEventDispatcher() { return eventDispatcher; }

        void update(float deltaTime) {
            ecs.update(deltaTime);
        }

        template<typename T, typename... Args>
        void registerSystem(Args&&... args) {
            ecs.registerSystem<T>(std::forward<Args>(args)...);
        }
    };

    class WorldBox {
        std::vector<std::shared_ptr<World>> worlds;
    public:
        auto& getWorlds() { return worlds; }

        template<typename Component>
        void registerComponent() {
            for (auto const& w : worlds) {
                w->getECS().registerComponent<Component>();
            }
        }
    };
}

#endif //METEORSHIP_WORLD_H
