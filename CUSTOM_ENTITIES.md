# Custom entities

Custom definitions are loaded at startup from `assets/entities/{spells,turrets,enemies}/*.json`.
Lua scripts referenced by definitions are executable game code and should only be loaded from
trusted sources. Script paths are relative to the game working directory and functions are
registered as globals by the script.

## Spells

Each spell has an `id`, `name`, `type` (`projectile`, `static_projectile`, `modifier`,
`material`, `multicast`, `utility`, or `other`), a `stats` object, and
optional `icon` and `projectile_sprite` asset names. Supported stats are `damage`, `speed`,
`life_time`, `mana_drain`, `cast_delay`, `recharge_time`, `spread_modification`, `radius`,
`crit_change_bonus`, `uses`, `friendly_fire`, `piercing`, `penetrating`, `trigger` (`none`,
`collision`, `timer`, or `expiration`), and `timer_life_time`.

Modifier spells can provide a `script` and a `behavior` object with any of `on_init`,
`on_update`, `on_collision`, and `on_death` set to global Lua function names. Each function
receives a `ModifierContext` (`world`, `projectile_entity`, `target_entity`, `hit_normal`,
`delta_time`). Projectile and static-projectile spells can also use those hooks. A multicast
spell accepts `draw` and optional `formation` (`spread`, `bi_directional`, `tri_directional`,
`above_and_below`, `hexagonal`, `behind`, or `none`), plus `spread_angle_degrees`. A utility
spell uses a Lua `function` name; that function is called without arguments.

## Turrets

Turrets have `id`, `name`, optional `icon`, `spell_slots`, `target_mode` (`automatic` or
`manual`), `stats`, optional `spells` (an
ordered array of spell IDs), and optional `script` / `on_fire`. Turret stats are
`shuffle`, `spells_per_cast`, `cast_delay`, `recharge_time`, `mana_max`,
`mana_charge_speed`, `capacity`, `spread`, and `speed_multiplier`. The `on_fire` Lua function
receives `(world, owner_entity, origin, aim_angle_radians)` before the turret fires its deck.
Returning `false` skips the built-in deck shot; return `true` to allow it.

## Enemies and Lua behavior trees

Enemies have `id`, `name`, a `sprite` asset name, and a `behavior_tree` path to a
BehaviorTree.CPP XML file. Author the tree in Groot2 as usual. Register `LuaAction` or
`LuaCondition` in the tree; each takes a `script` path, a Lua `function` name, optional
`parameters`, and the `bt_context` blackboard input. This keeps tree structure and node
placement in Groot2's XML while letting Lua implement the actual behavior.

An action Lua function receives `(context, parameters)` and returns `"SUCCESS"`, `"FAILURE"`,
or `"RUNNING"`. A condition returns a boolean. The context contains the current `world`,
`entity`, optional `player_entity`, and simulation `delta_time`. `parameters` is a JSON object
encoded as `json:{...}` in the XML attribute (for example
`parameters="json:{&quot;speed&quot;:45}"`). The `scout.json`, `normal_enemy.xml`, and
`scout.lua` files show a complete Groot2-authored tree using Lua nodes.
