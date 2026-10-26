//
// Created by ricardo on 17.10.2026.
//
#include "World.h"
#include "Game.h"

namespace ms {
    World::World(Game& game)
        : m_quadtree({{0.f, 0.f}, {static_cast<float>(game.getMainWindow().getSize().x), static_cast<float>(game.getMainWindow().getSize().y)}}) {}
}
