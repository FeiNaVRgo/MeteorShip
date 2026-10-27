//
// Created by iformat on 10/15/26.
//

#include "SceneBase.h"
#include <utility>
#include "Game.h"


namespace ms::scene_logic::scenes {
    SceneBase::SceneBase(Game& game, std::string name)
        : name(std::move(name)),
          game(game),
          world(std::make_shared<World>(game))
    {

    }

    void SceneBase::drawUI(nk_context *ctx) {ui.draw(ctx, game.getMainWindow().getSize());}
}
