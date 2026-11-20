#include "CustomEntityLoader.h"

#include <boost/json.hpp>
#include <sol/sol.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "BTNodes/BTContext.h"
#include "Components/BehaviorTree.h"
#include "Components/Player.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Game.h"
#include "Utility/AssetManager.h"

namespace ms::utility {
    namespace {
        namespace json = boost::json;

        json::object readDefinition(const std::filesystem::path& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) throw std::runtime_error("Failed to open entity definition: " + path.string());
            std::stringstream contents;
            contents << file.rdbuf();
            try {
                const auto parsed = json::parse(contents.str());
                if (!parsed.is_object()) {
                    throw std::runtime_error("the root must be a JSON object");
                }
                return parsed.as_object();
            } catch (const std::exception& error) {
                throw std::runtime_error("Invalid entity definition '" + path.string() + "': " +
                                         error.what());
            }
        }

        const json::value* find(const json::object& object, const std::string_view key) {
            return object.if_contains(json::string_view(key.data(), key.size()));
        }

        std::vector<std::filesystem::path> definitionFiles(const std::string& category) {
            const std::filesystem::path directory = std::filesystem::path("./assets/entities") / category;
            if (!std::filesystem::exists(directory)) return {};
            if (!std::filesystem::is_directory(directory)) {
                throw std::runtime_error("Entity definition path is not a directory: " +
                                         directory.string());
            }
            std::vector<std::filesystem::path> files;
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    files.push_back(entry.path());
                }
            }
            std::ranges::sort(files);
            return files;
        }

        const json::value& required(const json::object& object, const std::string_view key,
                                   const std::filesystem::path& path) {
            if (const auto* value = find(object, key)) return *value;
            throw std::runtime_error("Missing '" + std::string(key) + "' in " + path.string());
        }

        std::string stringValue(const json::value& value, const std::string& field,
                                const std::filesystem::path& path) {
            if (!value.is_string()) {
                throw std::runtime_error("'" + field + "' must be a string in " + path.string());
            }
            const auto& text = value.as_string();
            return std::string(text.data(), text.size());
        }

        float numberValue(const json::value& value, const std::string& field,
                          const std::filesystem::path& path) {
            if (!value.is_number()) {
                throw std::runtime_error("'" + field + "' must be a number in " + path.string());
            }
            const double number = value.is_double() ? value.as_double()
                : value.is_int64() ? static_cast<double>(value.as_int64())
                : static_cast<double>(value.as_uint64());
            if (!std::isfinite(number) ||
                number < -std::numeric_limits<float>::max() ||
                number > std::numeric_limits<float>::max()) {
                throw std::runtime_error("'" + field + "' must be a finite float in " + path.string());
            }
            return static_cast<float>(number);
        }

        float optionalNumber(const json::object& object, const std::string_view key, float fallback,
                             const std::filesystem::path& path) {
            const auto* value = find(object, key);
            return value ? numberValue(*value, std::string(key), path) : fallback;
        }

        bool optionalBool(const json::object& object, const std::string_view key, bool fallback,
                          const std::filesystem::path& path) {
            const auto* value = find(object, key);
            if (!value) return fallback;
            if (!value->is_bool()) {
                throw std::runtime_error("'" + std::string(key) + "' must be a boolean in " +
                                         path.string());
            }
            return value->as_bool();
        }

        std::size_t nonNegativeIntegerValue(const json::value& value, const std::string& field,
                                            const std::filesystem::path& path) {
            if (value.is_uint64()) {
                if (value.as_uint64() > std::numeric_limits<std::size_t>::max()) {
                    throw std::runtime_error("'" + field + "' is too large in " + path.string());
                }
                return static_cast<std::size_t>(value.as_uint64());
            }
            if (value.is_int64()) {
                const auto number = value.as_int64();
                if (number < 0) {
                    throw std::runtime_error("'" + field + "' must be a non-negative integer in " +
                                             path.string());
                }
                return static_cast<std::size_t>(number);
            }
            if (!value.is_double()) {
                throw std::runtime_error("'" + field + "' must be a non-negative integer in " +
                                         path.string());
            }
            const double number = value.as_double();
            const double exclusiveUpperBound = std::ldexp(
                1.0, std::numeric_limits<std::size_t>::digits);
            if (!std::isfinite(number) || number < 0.0 || std::floor(number) != number ||
                number >= exclusiveUpperBound) {
                throw std::runtime_error("'" + field + "' must be a non-negative integer in " +
                                         path.string());
            }
            return static_cast<std::size_t>(number);
        }

        std::size_t optionalSize(const json::object& object, const std::string_view key,
                                 std::size_t fallback, const std::filesystem::path& path) {
            const auto* value = find(object, key);
            return value ? nonNegativeIntegerValue(*value, std::string(key), path) : fallback;
        }

        const json::object& objectValue(const json::value& value, const std::string& field,
                                        const std::filesystem::path& path) {
            if (!value.is_object()) {
                throw std::runtime_error("'" + field + "' must be an object in " + path.string());
            }
            return value.as_object();
        }

        sf::Sprite spriteFromAsset(Game& game, const json::object& object,
                                  const std::string_view key, const std::filesystem::path& path) {
            const auto& assetName = stringValue(required(object, key, path), std::string(key), path);
            const auto iterator = game.getAssetManager().spriteMap.find(assetName);
            if (iterator == game.getAssetManager().spriteMap.end()) {
                throw std::runtime_error("Unknown sprite asset '" + assetName + "' in " + path.string());
            }
            return iterator->second.sprite;
        }

        inventory::ProjectileStats readSpellStats(const json::object& object,
                                                  const std::filesystem::path& path,
                                                  const std::string& type) {
            inventory::ProjectileStats stats;
            if (const auto* value = object.if_contains("damage")) {
                stats.damage = numberValue(*value, "damage", path);
            }
            if (const auto* value = object.if_contains("speed")) {
                stats.speed = numberValue(*value, "speed", path);
            }
            const bool isProjectile = type == "projectile" || type == "static_projectile" ||
                                      type == "material" || type == "other";
            stats.lifeTime = optionalNumber(object, "life_time", isProjectile ? 5.f : 0.f, path);
            if (const auto* value = object.if_contains("mana_drain")) {
                stats.manaDrain = numberValue(*value, "mana_drain", path);
            }
            if (const auto* value = object.if_contains("cast_delay")) {
                stats.castDelay = numberValue(*value, "cast_delay", path);
            }
            if (const auto* value = object.if_contains("recharge_time")) {
                stats.rechargeTime = numberValue(*value, "recharge_time", path);
            }
            if (const auto* value = object.if_contains("spread_modification")) {
                stats.spreadModification = numberValue(*value, "spread_modification", path);
            }
            if (const auto* value = object.if_contains("radius")) {
                stats.radius = numberValue(*value, "radius", path);
            }
            if (const auto* value = object.if_contains("crit_change_bonus")) {
                stats.critChangeBonus = numberValue(*value, "crit_change_bonus", path);
            }
            if (const auto* value = object.if_contains("uses")) {
                const auto uses = nonNegativeIntegerValue(*value, "uses", path);
                if (uses > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                    throw std::runtime_error("'uses' is too large in " + path.string());
                }
                stats.uses = static_cast<int>(uses);
            }
            stats.friendlyFire = optionalBool(object, "friendly_fire", false, path);
            stats.Piercing = optionalBool(object, "piercing", false, path);
            stats.Penetrating = optionalBool(object, "penetrating", false, path);
            if (const auto* value = object.if_contains("trigger")) {
                const auto trigger = stringValue(*value, "trigger", path);
                if (trigger == "collision") stats.triggerType = inventory::TriggerType::Collision;
                else if (trigger == "timer") stats.triggerType = inventory::TriggerType::Timer;
                else if (trigger == "expiration") stats.triggerType = inventory::TriggerType::Expiration;
                else if (trigger != "none") {
                    throw std::runtime_error("Unknown spell trigger '" + trigger + "' in " + path.string());
                }
            }
            if (const auto* value = object.if_contains("timer_life_time")) {
                stats.timerLifeTime = numberValue(*value, "timer_life_time", path);
            }
            return stats;
        }

        inventory::ProjectileModifierHook makeSpellHooks(Game& game, const json::object& definition,
                                                         const std::filesystem::path& path) {
            inventory::ProjectileModifierHook hooks;
            const auto* behavior = definition.if_contains("behavior");
            if (!behavior) return hooks;
            const auto& functions = objectValue(*behavior, "behavior", path);
            const auto loadFunction = [&game, &path, &functions](std::string_view key)
                -> std::optional<sol::protected_function> {
                const auto* functionName = find(functions, key);
                if (!functionName) return std::nullopt;
                const auto name = stringValue(*functionName, std::string(key), path);
                sol::object object = game.getLuaState()[name];
                if (!object.valid() || object.get_type() != sol::type::function) {
                    throw std::runtime_error("Lua spell behavior function '" + name +
                                             "' is not defined in " + path.string());
                }
                return object.as<sol::protected_function>();
            };
            const auto assign = [](auto& hook, std::optional<sol::protected_function> function) {
                if (!function) return;
                hook = [function = std::move(*function)](inventory::ModifierContext& context) mutable {
                    const auto result = function(context);
                    if (!result.valid()) {
                        const sol::error error = result;
                        throw std::runtime_error("Lua spell behavior failed: " + std::string(error.what()));
                    }
                };
            };
            assign(hooks.onInit, loadFunction("on_init"));
            assign(hooks.onUpdate, loadFunction("on_update"));
            assign(hooks.onCollision, loadFunction("on_collision"));
            assign(hooks.onDeath, loadFunction("on_death"));
            return hooks;
        }

        inventory::TurretStats readTurretStats(const json::object& object,
                                               const std::filesystem::path& path) {
            inventory::TurretStats stats;
            stats.shuffle = optionalBool(object, "shuffle", stats.shuffle, path);
            stats.spellsPerCast = optionalSize(object, "spells_per_cast", stats.spellsPerCast, path);
            stats.castDelay = optionalNumber(object, "cast_delay", stats.castDelay, path);
            stats.rechargeTime = optionalNumber(object, "recharge_time", stats.rechargeTime, path);
            stats.manaMax = optionalNumber(object, "mana_max", stats.manaMax, path);
            stats.manaChargeSpeed = optionalNumber(object, "mana_charge_speed", stats.manaChargeSpeed, path);
            stats.capacity = optionalSize(object, "capacity", stats.capacity, path);
            stats.spread = optionalNumber(object, "spread", stats.spread, path);
            stats.speedMultiplier = optionalNumber(object, "speed_multiplier", stats.speedMultiplier, path);
            return stats;
        }

        inventory::FormationType formationType(const std::string& value,
                                               const std::filesystem::path& path) {
            if (value == "spread") return inventory::FormationType::Spread;
            if (value == "bi_directional") return inventory::FormationType::BiDirectional;
            if (value == "tri_directional") return inventory::FormationType::TriDirectional;
            if (value == "above_and_below") return inventory::FormationType::AboveAndBelow;
            if (value == "hexagonal") return inventory::FormationType::Hexagonal;
            if (value == "behind") return inventory::FormationType::Behind;
            if (value == "none") return inventory::FormationType::None;
            throw std::runtime_error("Unknown multicast formation '" + value + "' in " + path.string());
        }

    }

    std::vector<CustomSpellDefinition> CustomEntityLoader::loadSpells() const {
        std::vector<CustomSpellDefinition> definitions;
        std::unordered_set<std::string> ids;
        for (const auto& path : definitionFiles("spells")) {
            const auto definition = readDefinition(path);
            const auto id = stringValue(required(definition, "id", path), "id", path);
            const auto name = stringValue(required(definition, "name", path), "name", path);
            if (!ids.insert(id).second) throw std::runtime_error("Duplicate spell id: " + id);
            if (const auto* script = definition.if_contains("script")) {
                game.loadLuaScript(stringValue(*script, "script", path));
            }
            const auto type = definition.if_contains("type")
                ? stringValue(*definition.if_contains("type"), "type", path)
                : "projectile";
            const auto& statsObject = objectValue(required(definition, "stats", path), "stats", path);
            auto stats = readSpellStats(statsObject, path, type);

            const auto hooks = makeSpellHooks(game, definition, path);
            inventory::ProjectileType projectile = inventory::Projectile{};
            if (type == "projectile") {
                projectile = inventory::Projectile{stats, hooks};
            } else if (type == "static_projectile") {
                projectile = inventory::StaticProjectile{stats, hooks};
            } else if (type == "modifier") {
                projectile = inventory::ProjectileModifier{hooks, stats};
            } else if (type == "material") {
                projectile = inventory::Material{stats};
            } else if (type == "other") {
                projectile = inventory::Other{stats};
            } else if (type == "multicast") {
                const auto drawCount = optionalSize(definition, "draw", 2, path);
                if (drawCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                    throw std::runtime_error("'draw' is too large in " + path.string());
                }
                const int draw = static_cast<int>(drawCount);
                std::optional<inventory::SpellFormation> formation;
                if (const auto* formationValue = definition.if_contains("formation")) {
                    const auto name = stringValue(*formationValue, "formation", path);
                    formation.emplace(
                        formationType(name, path),
                        optionalNumber(definition, "spread_angle_degrees", 0.f, path));
                }
                projectile = inventory::Multicast{draw, formation, stats};
            } else if (type == "utility") {
                const auto functionName = stringValue(required(definition, "function", path),
                                                      "function", path);
                const sol::object object = game.getLuaState()[functionName];
                if (!object.valid() || object.get_type() != sol::type::function) {
                    throw std::runtime_error("Lua utility spell function '" + functionName +
                                             "' is not defined in " + path.string());
                }
                auto function = object.as<sol::protected_function>();
                projectile = inventory::Utility{
                    [function = std::move(function)]() mutable {
                        const auto result = function();
                        if (!result.valid()) {
                            const sol::error error = result;
                            throw std::runtime_error("Lua utility spell failed: " +
                                                     std::string(error.what()));
                        }
                    },
                    stats
                };
            } else {
                throw std::runtime_error("Unknown spell type '" + type + "' in " + path.string());
            }

            std::optional<sf::Sprite> icon;
            if (definition.if_contains("icon")) icon = spriteFromAsset(game, definition, "icon", path);
            inventory::Spell spell{name, std::move(projectile), std::move(icon)};
            if (const auto* sprite = definition.if_contains("projectile_sprite")) {
                const auto spriteName = stringValue(*sprite, "projectile_sprite", path);
                const auto iterator = game.getAssetManager().spriteMap.find(spriteName);
                if (iterator == game.getAssetManager().spriteMap.end()) {
                    throw std::runtime_error("Unknown projectile sprite '" + spriteName + "' in " +
                                             path.string());
                }
                spell.setProjectileSprite(iterator->second);
            }
            definitions.push_back({id, std::move(spell)});
        }
        return definitions;
    }

    std::vector<CustomTurretDefinition> CustomEntityLoader::loadTurrets() const {
        std::vector<CustomTurretDefinition> definitions;
        std::unordered_set<std::string> ids;
        for (const auto& path : definitionFiles("turrets")) {
            const auto definition = readDefinition(path);
            const auto id = stringValue(required(definition, "id", path), "id", path);
            const auto name = stringValue(required(definition, "name", path), "name", path);
            if (!ids.insert(id).second) throw std::runtime_error("Duplicate turret id: " + id);
            if (const auto* script = definition.if_contains("script")) {
                game.loadLuaScript(stringValue(*script, "script", path));
            }
            const auto& statsObject = objectValue(required(definition, "stats", path), "stats", path);
            const auto stats = readTurretStats(statsObject, path);
            const auto spellSlots = optionalSize(definition, "spell_slots", 0, path);
            std::optional<sf::Sprite> icon;
            if (definition.if_contains("icon")) icon = spriteFromAsset(game, definition, "icon", path);
            inventory::Turret turret{name, std::move(icon), spellSlots, stats};
            if (const auto* mode = definition.if_contains("target_mode")) {
                const auto targetMode = stringValue(*mode, "target_mode", path);
                if (targetMode == "manual") {
                    turret.setTargetMode(inventory::TurretTargetMode::Manual);
                } else if (targetMode != "automatic") {
                    throw std::runtime_error("Unknown turret target mode '" + targetMode + "' in " +
                                             path.string());
                }
            }

            if (const auto* callback = definition.if_contains("on_fire")) {
                const auto functionName = stringValue(*callback, "on_fire", path);
                sol::object object = game.getLuaState()[functionName];
                if (!object.valid() || object.get_type() != sol::type::function) {
                    throw std::runtime_error("Lua turret behavior function '" + functionName +
                                             "' is not defined in " + path.string());
                }
                auto function = object.as<sol::protected_function>();
                turret.setOnFireHook(
                    [function = std::move(function)](World& world, ecs::Entity owner,
                                                     sf::Vector2f origin, float aimAngle) mutable {
                        const auto result = function(&world, owner, origin, aimAngle);
                        if (!result.valid()) {
                            const sol::error error = result;
                            throw std::runtime_error("Lua turret behavior failed: " +
                                                     std::string(error.what()));
                        }
                        return result.get<bool>();
                    });
            }

            std::vector<std::string> spellIds;
            if (const auto* value = definition.if_contains("spells")) {
                if (!value->is_array()) {
                    throw std::runtime_error("'spells' must be an array in " + path.string());
                }
                for (const auto& spell : value->as_array()) {
                    spellIds.push_back(stringValue(spell, "spells[]", path));
                }
            }
            definitions.push_back({id, std::move(turret), std::move(spellIds)});
        }
        return definitions;
    }

    std::vector<CustomEnemyDefinition> CustomEntityLoader::loadEnemies() const {
        std::vector<CustomEnemyDefinition> definitions;
        std::unordered_set<std::string> ids;
        for (const auto& path : definitionFiles("enemies")) {
            const auto definition = readDefinition(path);
            const auto id = stringValue(required(definition, "id", path), "id", path);
            const auto name = stringValue(required(definition, "name", path), "name", path);
            const auto sprite = stringValue(required(definition, "sprite", path), "sprite", path);
            if (!ids.insert(id).second) throw std::runtime_error("Duplicate enemy id: " + id);
            const auto behaviorTree = stringValue(required(definition, "behavior_tree", path),
                                                  "behavior_tree", path);
            if (!std::filesystem::is_regular_file(behaviorTree)) {
                throw std::runtime_error("Enemy behavior tree XML does not exist: " + behaviorTree);
            }
            if (game.getAssetManager().spriteMap.find(sprite) == game.getAssetManager().spriteMap.end()) {
                throw std::runtime_error("Unknown enemy sprite asset '" + sprite + "' in " + path.string());
            }
            definitions.push_back({id, name, sprite, behaviorTree});
        }
        return definitions;
    }

    components::BehaviorTree CustomEntityLoader::createEnemyBehaviorTree(
        Game& game, const std::shared_ptr<World>& world, const ecs::Entity entity,
        const std::string& behaviorTree) {
        auto blackboard = BT::Blackboard::create(game.getGlobalBlackboard());
        std::optional<ecs::Entity> playerEntity;
        world->getECS().view<components::Player, components::Transform>().each(
            [&playerEntity](const ecs::Entity& player, const components::Player&,
                            const components::Transform&) {
                if (!playerEntity) playerEntity = player;
            });
        blackboard->set("bt_context", bt_nodes::BTContext{
            .world = world,
            .entity = entity,
            .playerEntity = playerEntity
        });
        auto tree = game.getBehaviorTreeFactory().createTreeFromFile(behaviorTree, blackboard);
        return {
            .BTTree = std::move(tree),
            .blackboard = std::move(blackboard)
        };
    }
}
