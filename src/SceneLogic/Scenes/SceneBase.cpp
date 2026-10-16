//
// Created by iformat on 10/15/26.
//

#include "SceneBase.h"

#include <utility>
#include "Game.h"

namespace ms::scene_logic::scenes {
    SceneBase::SceneBase(Game& game, std::string name) : game(game), name(std::move(name)) {}
}