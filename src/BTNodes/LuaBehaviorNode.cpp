#include "LuaBehaviorNode.h"

#include <boost/json.hpp>
#include <sol/sol.hpp>

#include <memory>
#include <stdexcept>
#include <string>

#include "BTContext.h"
#include "ECS/World.h"
#include "Game.h"

namespace ms::bt_nodes {
    namespace {
        sol::object toLua(sol::state& lua, const boost::json::value& value) {
            if (value.is_null()) return sol::make_object(lua, sol::nil);
            if (value.is_bool()) return sol::make_object(lua, value.as_bool());
            if (value.is_string()) return sol::make_object(lua, std::string(value.as_string()));
            if (value.is_int64()) return sol::make_object(lua, value.as_int64());
            if (value.is_uint64()) return sol::make_object(lua, value.as_uint64());
            if (value.is_double()) return sol::make_object(lua, value.as_double());

            sol::table table = lua.create_table();
            if (value.is_array()) {
                const auto& array = value.as_array();
                for (std::size_t index = 0; index < array.size(); ++index) {
                    table[index + 1] = toLua(lua, array[index]);
                }
            } else if (value.is_object()) {
                for (const auto& [key, child] : value.as_object()) {
                    table[std::string(key.data(), key.size())] = toLua(lua, child);
                }
            }
            return sol::make_object(lua, table);
        }

        sol::table makeContext(sol::state& lua, const BTContext& context) {
            const auto world = context.world.lock();
            if (!world) throw std::runtime_error("Lua behavior node has no live world.");
            sol::table result = lua.create_table();
            result["world"] = world.get();
            result["entity"] = context.entity;
            result["player_entity"] = context.playerEntity
                ? sol::make_object(lua, *context.playerEntity)
                : sol::make_object(lua, sol::nil);
            result["delta_time"] = context.deltaTime;
            return result;
        }

        sol::protected_function getFunction(sol::state& lua, const std::string& name) {
            const sol::object object = lua[name];
            if (!object.valid() || object.get_type() != sol::type::function) {
                throw std::runtime_error("Lua behavior function is not defined: " + name);
            }
            return object.as<sol::protected_function>();
        }

        sol::table getParameters(sol::state& lua, const std::string& encoded) {
            constexpr std::string_view prefix = "json:";
            const std::string_view jsonText = encoded.starts_with(prefix)
                ? std::string_view(encoded).substr(prefix.size())
                : std::string_view(encoded);
            const auto parameters = boost::json::parse(jsonText);
            if (!parameters.is_object()) {
                throw std::runtime_error("Lua behavior parameters must be a JSON object.");
            }
            return toLua(lua, parameters).as<sol::table>();
        }

        BT::NodeStatus luaStatus(const sol::protected_function_result& result,
                                 const std::string& functionName) {
            if (!result.valid()) {
                const sol::error error = result;
                throw std::runtime_error("Lua behavior function '" + functionName + "' failed: " +
                                         error.what());
            }
            const std::string status = result.get<std::string>();
            if (status == "SUCCESS") return BT::NodeStatus::SUCCESS;
            if (status == "FAILURE") return BT::NodeStatus::FAILURE;
            if (status == "RUNNING") return BT::NodeStatus::RUNNING;
            throw std::runtime_error("Lua behavior function '" + functionName +
                                     "' must return SUCCESS, FAILURE, or RUNNING.");
        }

        class LuaAction final : public BT::StatefulActionNode {
            Game& game;

        public:
            LuaAction(const std::string& name, const BT::NodeConfig& config, Game& game)
                : BT::StatefulActionNode(name, config), game(game) {}

            static BT::PortsList providedPorts() {
                return {
                    BT::InputPort<BTContext>("bt_context"),
                    BT::InputPort<std::string>("script"),
                    BT::InputPort<std::string>("function"),
                    BT::InputPort<std::string>("parameters", "json:{}")
                };
            }

            BT::NodeStatus run() {
                const auto context = getInput<BTContext>("bt_context");
                const auto script = getInput<std::string>("script");
                const auto functionName = getInput<std::string>("function");
                const auto parameters = getInput<std::string>("parameters");
                if (!context || !script || !functionName || !parameters) {
                    throw std::runtime_error("LuaAction is missing a required input port.");
                }
                game.loadLuaScript(script.value());
                auto& lua = game.getLuaState();
                auto function = getFunction(lua, functionName.value());
                auto luaContext = makeContext(lua, context.value());
                auto luaParameters = getParameters(lua, parameters.value());
                return luaStatus(function(luaContext, luaParameters), functionName.value());
            }

            BT::NodeStatus onStart() override { return run(); }
            BT::NodeStatus onRunning() override { return run(); }
            void onHalted() override {}
        };

        class LuaCondition final : public BT::ConditionNode {
            Game& game;

        public:
            LuaCondition(const std::string& name, const BT::NodeConfig& config, Game& game)
                : BT::ConditionNode(name, config), game(game) {}

            static BT::PortsList providedPorts() {
                return {
                    BT::InputPort<BTContext>("bt_context"),
                    BT::InputPort<std::string>("script"),
                    BT::InputPort<std::string>("function"),
                    BT::InputPort<std::string>("parameters", "json:{}")
                };
            }

            BT::NodeStatus tick() override {
                const auto context = getInput<BTContext>("bt_context");
                const auto script = getInput<std::string>("script");
                const auto functionName = getInput<std::string>("function");
                const auto parameters = getInput<std::string>("parameters");
                if (!context || !script || !functionName || !parameters) {
                    throw std::runtime_error("LuaCondition is missing a required input port.");
                }
                game.loadLuaScript(script.value());
                auto& lua = game.getLuaState();
                auto function = getFunction(lua, functionName.value());
                auto luaContext = makeContext(lua, context.value());
                auto luaParameters = getParameters(lua, parameters.value());
                const auto result = function(luaContext, luaParameters);
                if (!result.valid()) {
                    const sol::error error = result;
                    throw std::runtime_error("Lua behavior function '" + functionName.value() +
                                             "' failed: " + error.what());
                }
                return result.get<bool>() ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
            }
        };
    }

    void registerLuaBehaviorNodes(BT::BehaviorTreeFactory& factory, Game& game) {
        factory.registerBuilder<LuaAction>(
            "LuaAction", [&game](const std::string& name, const BT::NodeConfig& config) {
                return std::make_unique<LuaAction>(name, config, game);
            });
        factory.registerBuilder<LuaCondition>(
            "LuaCondition", [&game](const std::string& name, const BT::NodeConfig& config) {
                return std::make_unique<LuaCondition>(name, config, game);
            });
    }
}
