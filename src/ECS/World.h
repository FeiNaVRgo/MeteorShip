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
    namespace systems {
        class OpenCLCollisionDetector;
    }

    class World {
        Game& game;
        ecs::ECS ecs;
        systems::Quadtree m_quadtree;
        EventDispatcher eventDispatcher;
    public:
        explicit World(Game& game);

        [[nodiscard]] Game& getGame() const;
        ecs::ECS& getECS();
        systems::Quadtree& getQuadtree();
        EventDispatcher& getEventDispatcher();
        [[nodiscard]] sf::RenderWindow& getMainWindow() const;
        [[nodiscard]] systems::OpenCLCollisionDetector& getCollisionDetector() const;

        void update(const float deltaTime) {
            ecs.update(deltaTime);
        }

        template<typename T, typename... Args>
        T& registerSystem(Args&&... args) {
            return ecs.registerSystem<T>(*this, std::forward<Args>(args)...);
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
