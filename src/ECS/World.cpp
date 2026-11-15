//
// Created by ricardo on 17.10.2026.
//
#include "World.h"
#include "Game.h"
#include "Systems/Physics/OpenCLCollisionDetector.h"

namespace ms {
    World::World(Game& game)
        : game(game),
          m_quadtree({{0.f, 0.f}, {
              static_cast<float>(game.getMainWindow().getSize().x),
              static_cast<float>(game.getMainWindow().getSize().y)}}) {}

    Game& World::getGame() const { return game; }

    ecs::ECS& World::getECS() { return ecs; }

    systems::Quadtree& World::getQuadtree() { return m_quadtree; }

    EventDispatcher& World::getEventDispatcher() { return eventDispatcher; }

    sf::RenderWindow& World::getMainWindow() const {
        return game.getMainWindow();
    }

    systems::OpenCLCollisionDetector& World::getCollisionDetector() const {
        return game.getCollisionDetector();
    }
}
