//
// Created by iformat on 11/17/26.
//

#ifndef METEORSHIP_LUA_BINDINGS_H
#define METEORSHIP_LUA_BINDINGS_H
#include <sol/state.hpp>

#include "Game.h"

#include "lua_game_bindings.h"
#include "lua_sfml_bindings.h"

namespace ms::scripting {
    inline void lua_bind(sol::state& lua, Game& game) {
        lua_sfml_bind(lua);
        lua_game_bind(lua, game);
    }
}

#endif //METEORSHIP_LUA_BINDINGS_H
