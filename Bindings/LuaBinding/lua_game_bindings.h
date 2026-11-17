#ifndef METEORSHIP_LUA_GAME_BINDINGS_H
#define METEORSHIP_LUA_GAME_BINDINGS_H

#include <sol/sol.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "Components/BehaviorTree.h"
#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Damage.h"
#include "Components/Drawable.h"
#include "Components/Enemy.h"
#include "Components/Hitbox.h"
#include "Components/Player.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Scene.h"
#include "Components/Transform.h"
#include "Components/TurretMountLayout.h"
#include "ECS/World.h"
#include "Events/EventHandling.h"
#include "Game.h"
#include "Inventory/Spell.h"
#include "SceneLogic/SceneManager.h"
#include "SceneLogic/Scenes/SceneBase.h"
#include "Utility/AssetManager.h"

namespace ms::scripting {
    template<typename T>
    auto make_event_subscriber() {
        return [](utility::EventHandling& handler, sol::function callback) {
            return handler.bindToSignal<T>([callback = std::move(callback)](void* data) mutable {
                const T eventCopy = *static_cast<const T*>(data);
                callback(eventCopy);
            });
        };
    }

    template<typename T>
    void bind_component_access(sol::usertype<ecs::ECS>& ecsType, const std::string& name) {
        ecsType["add_" + name] = [](ecs::ECS& ecs, ecs::Entity entity) -> T& {
            return ecs.addComponent<T>(entity);
        };
        ecsType["has_" + name] = [](ecs::ECS& ecs, ecs::Entity entity) {
            return ecs.hasComponent<T>(entity);
        };
        ecsType["get_" + name] = [name](ecs::ECS& ecs, ecs::Entity entity) -> T& {
            if (!ecs.isEntityAlive(entity) || !ecs.hasComponent<T>(entity)) {
                throw std::runtime_error("entity has no " + name + " component");
            }
            return ecs.getComponent<T>(entity);
        };
        ecsType["remove_" + name] = [](ecs::ECS& ecs, ecs::Entity entity) {
            ecs.removeComponent<T>(entity);
        };
        ecsType["entities_with_" + name] = [](ecs::ECS& ecs) {
            std::vector<ecs::Entity> entities;
            ecs.forEach<T>([&entities](ecs::Entity entity, T&) {
                entities.push_back(entity);
            });
            return sol::as_table(entities);
        };
        ecsType["for_each_" + name] = [](ecs::ECS& ecs, sol::function callback) {
            ecs.forEach<T>([callback = std::move(callback)](ecs::Entity entity, T& component) mutable {
                callback(entity, component);
            });
        };
    }

    inline void lua_game_bind(sol::state& lua, Game& game) {
        using namespace components;

        lua.new_enum("ComponentDamageType",
            "Projectile", DamageType::Projectile,
            "Fire", DamageType::Fire,
            "Frost", DamageType::Frost,
            "Electric", DamageType::Electric,
            "Poison", DamageType::Poison,
            "Explosion", DamageType::Explosion);
        lua.new_enum("ProjectileTriggerType",
            "None", inventory::TriggerType::None,
            "Collision", inventory::TriggerType::Collision,
            "Timer", inventory::TriggerType::Timer,
            "Expiration", inventory::TriggerType::Expiration);
        lua.new_enum("SpellFormationType",
            "None", inventory::FormationType::None,
            "Spread", inventory::FormationType::Spread,
            "BiDirectional", inventory::FormationType::BiDirectional,
            "TriDirectional", inventory::FormationType::TriDirectional,
            "AboveAndBelow", inventory::FormationType::AboveAndBelow,
            "Hexagonal", inventory::FormationType::Hexagonal,
            "Behind", inventory::FormationType::Behind);

        lua.new_usertype<DamageAmount>("DamageAmount",
            sol::constructors<sol::types<>>(),
            "type", &DamageAmount::type,
            "amount", &DamageAmount::amount);
        lua["DamageAmount"]["DamageType"] = lua.get<sol::object>("ComponentDamageType");

        lua.new_usertype<inventory::ProjectileStats>("ProjectileStats",
            sol::constructors<sol::types<>>(),
            "uses", &inventory::ProjectileStats::uses,
            "mana_drain", &inventory::ProjectileStats::manaDrain,
            "cast_delay", &inventory::ProjectileStats::castDelay,
            "recharge_time", &inventory::ProjectileStats::rechargeTime,
            "spread_modification", &inventory::ProjectileStats::spreadModification,
            "radius", &inventory::ProjectileStats::radius,
            "speed", &inventory::ProjectileStats::speed,
            "crit_change_bonus", &inventory::ProjectileStats::critChangeBonus,
            "damage", &inventory::ProjectileStats::damage,
            "life_time", &inventory::ProjectileStats::lifeTime,
            "timer_life_time", &inventory::ProjectileStats::timerLifeTime,
            "friendly_fire", &inventory::ProjectileStats::friendlyFire,
            "piercing", &inventory::ProjectileStats::Piercing,
            "penetrating", &inventory::ProjectileStats::Penetrating,
            "trigger_type", &inventory::ProjectileStats::triggerType);
        lua["ProjectileStats"]["TriggerType"] = lua.get<sol::object>("ProjectileTriggerType");

        lua.new_usertype<inventory::SpellFormation>("SpellFormation",
            sol::constructors<sol::types<>, sol::types<inventory::FormationType>,
                              sol::types<inventory::FormationType, float>>(),
            "type", &inventory::SpellFormation::type,
            "spread_angle_degrees", &inventory::SpellFormation::spreadAngleDegrees,
            "calculate_angles", [](const inventory::SpellFormation& formation, std::size_t count, float baseAngle) {
                return sol::as_table(formation.calculateAngles(count, baseAngle));
            });
        lua["SpellFormation"]["FormationType"] = lua.get<sol::object>("SpellFormationType");

        lua.new_usertype<inventory::ProjectileModifierHook>("ProjectileModifierHook",
            sol::constructors<sol::types<>>(),
            "has_any_hook", &inventory::ProjectileModifierHook::hasAnyHook,
            "set_on_init", [](inventory::ProjectileModifierHook& hook, sol::function callback) {
                hook.onInit = [callback = std::move(callback)](inventory::ModifierContext& context) mutable {
                    callback(context);
                };
            },
            "set_on_update", [](inventory::ProjectileModifierHook& hook, sol::function callback) {
                hook.onUpdate = [callback = std::move(callback)](inventory::ModifierContext& context) mutable {
                    callback(context);
                };
            },
            "set_on_collision", [](inventory::ProjectileModifierHook& hook, sol::function callback) {
                hook.onCollision = [callback = std::move(callback)](inventory::ModifierContext& context) mutable {
                    callback(context);
                };
            },
            "set_on_death", [](inventory::ProjectileModifierHook& hook, sol::function callback) {
                hook.onDeath = [callback = std::move(callback)](inventory::ModifierContext& context) mutable {
                    callback(context);
                };
            });

        lua.new_usertype<inventory::Spell>("Spell",
            sol::constructors<sol::types<>>(),
            "name", sol::property(&inventory::Spell::getName),
            "stats", sol::property([](inventory::Spell& spell) -> inventory::ProjectileStats& {
                return spell.getStats();
            }),
            "is_modifier", &inventory::Spell::isModifier,
            "is_multicast", &inventory::Spell::isMulticast,
            "is_projectile", &inventory::Spell::isProjectile,
            "has_trigger", &inventory::Spell::hasTrigger);
        lua.new_usertype<inventory::ShootableSpell>("ShootableSpell",
            sol::constructors<sol::types<>>(),
            "spell", &inventory::ShootableSpell::spell,
            "final_stats", &inventory::ShootableSpell::finalStats,
            "payload_formation", &inventory::ShootableSpell::payloadFormation,
            "payload_count", [](const inventory::ShootableSpell& value) { return value.payload.size(); },
            "get_payload", [](inventory::ShootableSpell& value, std::size_t index) -> inventory::ShootableSpell& {
                if (index >= value.payload.size()) throw std::out_of_range("spell payload index out of range");
                return value.payload[index];
            },
            "add_payload", [](inventory::ShootableSpell& value, const inventory::ShootableSpell& payload) {
                value.payload.push_back(payload);
            });

        lua.new_usertype<inventory::ModifierContext>("ModifierContext", sol::no_constructor,
            "world", sol::property([](inventory::ModifierContext& context) -> World& { return context.world; }),
            "projectile_entity", &inventory::ModifierContext::projectileEntity,
            "target_entity", &inventory::ModifierContext::targetEntity,
            "hit_normal", &inventory::ModifierContext::hitNormal,
            "delta_time", &inventory::ModifierContext::deltaTime);

        lua.new_usertype<BT::Tree>("BehaviorTreeHandle", sol::no_constructor,
            "tick_once", [](BT::Tree& tree) { (void)tree.tickOnce(); },
            "halt", &BT::Tree::haltTree);
        lua.new_usertype<BT::Blackboard>("Blackboard", sol::no_constructor,
            "get_bool", [](BT::Blackboard& blackboard, const std::string& key) {
                return blackboard.get<bool>(key);
            },
            "set_bool", [](BT::Blackboard& blackboard, const std::string& key, bool value) {
                blackboard.set(key, value);
            },
            "get_number", [](BT::Blackboard& blackboard, const std::string& key) {
                return blackboard.get<float>(key);
            },
            "set_number", [](BT::Blackboard& blackboard, const std::string& key, float value) {
                blackboard.set(key, value);
            },
            "get_string", [](BT::Blackboard& blackboard, const std::string& key) {
                return blackboard.get<std::string>(key);
            },
            "set_string", [](BT::Blackboard& blackboard, const std::string& key, const std::string& value) {
                blackboard.set(key, value);
            });
        lua.new_usertype<BehaviorTree>("BehaviorTree", sol::constructors<sol::types<>>(),
            "tree", sol::property([](BehaviorTree& value) -> BT::Tree& { return value.BTTree; }),
            "blackboard", sol::property([](BehaviorTree& value) { return value.blackboard; }),
            "has_blackboard", [](const BehaviorTree& value) { return static_cast<bool>(value.blackboard); });
        lua.new_usertype<BoundingBox>("BoundingBox", sol::constructors<sol::types<>>(),
            "bounds", &BoundingBox::bounds);
        lua.new_usertype<CollisionShape>("CollisionShape", sol::constructors<sol::types<>>(),
            "vertices", [](const CollisionShape& value) { return sol::as_table(value.getVertices()); },
            "vertex_count", [](const CollisionShape& value) { return value.getVertices().size(); });
        lua["CollisionShape"]["from_vertices"] = [&lua](sol::table values) {
            std::vector<sf::Vector2f> vertices;
            for (const auto& pair : values) vertices.push_back(pair.second.as<sf::Vector2f>());
            return CollisionShape(std::move(vertices));
        };
        lua.new_usertype<Drawable>("DrawableComponent", sol::constructors<sol::types<>>(),
            "has_vertices", [](const Drawable& value) { return value.vertices.has_value(); },
            "has_texture", [](const Drawable& value) { return value.texture.has_value(); },
            "has_sprite", [](const Drawable& value) { return value.sprite.has_value(); },
            "vertices", [](Drawable& value) -> sf::VertexArray* {
                return value.vertices ? &*value.vertices : nullptr;
            },
            "set_vertices", [](Drawable& value, const sf::VertexArray& vertices) { value.vertices = vertices; },
            "set_texture_from_file", [](Drawable& value, const std::string& path) {
                sf::Texture texture;
                if (!texture.loadFromFile(path)) throw std::runtime_error("failed to load texture: " + path);
                value.texture.emplace(std::move(texture));
            },
            "set_sprite", [](Drawable& value, const sf::Texture& texture, const sf::IntRect& rect, int frameCount) {
                value.sprite.emplace(texture, rect, frameCount, std::nullopt);
            },
            "sprite", [](Drawable& value) -> utility::AnimatedSprite* {
                return value.sprite ? &*value.sprite : nullptr;
            });
        lua.new_usertype<Enemy>("Enemy", sol::constructors<sol::types<>>());
        lua.new_usertype<OBB2D>("OBB2D",
            sol::constructors<sol::types<const sf::FloatRect&>,
                              sol::types<sf::Vector2f, sf::Vector2f, sf::Angle>>(),
            "center", &OBB2D::center,
            "extents", &OBB2D::extents,
            "update_axes", &OBB2D::updateAxes);
        lua.new_usertype<Hitbox>("Hitbox", sol::constructors<sol::types<>>(),
            "rectangle_count", [](const Hitbox& value) { return value.rectangles.size(); },
            "get_rectangle", [](Hitbox& value, std::size_t index) -> OBB2D& {
                if (index >= value.rectangles.size()) throw std::out_of_range("hitbox rectangle index out of range");
                return value.rectangles[index];
            },
            "add_rectangle", [](Hitbox& value, const OBB2D& rectangle) {
                value.rectangles.push_back(rectangle);
            },
            "clear", [](Hitbox& value) { value.rectangles.clear(); });
        lua.new_usertype<Player>("Player", sol::constructors<sol::types<>>());
        lua.new_usertype<utility::AnimatedSprite>("AnimatedSprite", sol::no_constructor,
            "sprite", &utility::AnimatedSprite::sprite,
            "rect", &utility::AnimatedSprite::rect,
            "number_of_sprites", &utility::AnimatedSprite::number_of_sprites,
            "update", &utility::AnimatedSprite::update,
            "create_instance", &utility::AnimatedSprite::createInstance);
        lua.new_usertype<Projectile>("Projectile", sol::constructors<sol::types<>>(),
            "owner", &Projectile::owner,
            "life_time", &Projectile::lifeTime,
            "age", &Projectile::age,
            "speed_multiplier", &Projectile::speedMultiplier,
            "friendly_fire", &Projectile::friendlyFire,
            "piercing", &Projectile::piercing,
            "penetrating", &Projectile::penetrating,
            "pending_removal", &Projectile::pendingRemoval,
            "trigger_type", &Projectile::triggerType,
            "payload_formation", &Projectile::payloadFormation,
            "timer", &Projectile::timer,
            "payload_count", [](const Projectile& value) { return value.payload.size(); },
            "get_payload", [](Projectile& value, std::size_t index) -> inventory::ShootableSpell& {
                if (index >= value.payload.size()) throw std::out_of_range("projectile payload index out of range");
                return value.payload[index];
            },
            "add_payload", [](Projectile& value, const inventory::ShootableSpell& payload) {
                value.payload.push_back(payload);
            },
            "modifier_hook_count", [](const Projectile& value) { return value.modifierHooks.size(); },
            "get_modifier_hook", [](Projectile& value, std::size_t index) -> inventory::ProjectileModifierHook& {
                if (index >= value.modifierHooks.size()) throw std::out_of_range("projectile modifier hook index out of range");
                return value.modifierHooks[index];
            },
            "add_modifier_hook", [](Projectile& value, const inventory::ProjectileModifierHook& hook) {
                value.modifierHooks.push_back(hook);
            },
            "has_hit_entity", [](const Projectile& value, ecs::Entity entity) { return value.hitEntities.contains(entity); },
            "mark_hit_entity", [](Projectile& value, ecs::Entity entity) { value.hitEntities.insert(entity); },
            "clear_hit_entities", [](Projectile& value) { value.hitEntities.clear(); });
        lua.new_usertype<Rigidbody>("Rigidbody", sol::constructors<sol::types<>>(),
            "velocity", &Rigidbody::velocity,
            "acceleration", &Rigidbody::acceleration);
        lua.new_usertype<Scene>("SceneComponent", sol::constructors<sol::types<>>());
        lua.new_usertype<Transform>("Transform", sol::constructors<sol::types<>>(),
            sol::base_classes, sol::bases<sf::Transformable>());
        lua.new_usertype<TurretMountLayout>("TurretMountLayout", sol::constructors<sol::types<>>(),
            "mount_count", [](const TurretMountLayout& value) { return value.topLeftOffsets.size(); },
            "get_mount", [](TurretMountLayout& value, std::size_t index) -> sf::Vector2f& {
                if (index >= value.topLeftOffsets.size()) throw std::out_of_range("turret mount index out of range");
                return value.topLeftOffsets[index];
            },
            "add_mount", [](TurretMountLayout& value, const sf::Vector2f& offset) {
                value.topLeftOffsets.push_back(offset);
            },
            "clear", [](TurretMountLayout& value) { value.topLeftOffsets.clear(); });

        auto ecsType = lua.new_usertype<ecs::ECS>("ECS", sol::no_constructor,
            "create_entity", &ecs::ECS::createEntity,
            "destroy_entity", &ecs::ECS::destroyEntity,
            "is_entity_alive", &ecs::ECS::isEntityAlive,
            "update", &ecs::ECS::update,
            "render", &ecs::ECS::render);
        bind_component_access<Rigidbody>(ecsType, "rigidbody");
        bind_component_access<BoundingBox>(ecsType, "bounding_box");
        bind_component_access<CollisionShape>(ecsType, "collision_shape");
        bind_component_access<Hitbox>(ecsType, "hitbox");
        bind_component_access<Transform>(ecsType, "transform");
        bind_component_access<Drawable>(ecsType, "drawable");
        bind_component_access<Player>(ecsType, "player");
        bind_component_access<BehaviorTree>(ecsType, "behavior_tree");
        bind_component_access<Enemy>(ecsType, "enemy");
        bind_component_access<TurretMountLayout>(ecsType, "turret_mount_layout");
        bind_component_access<Projectile>(ecsType, "projectile");
        bind_component_access<DamageAmount>(ecsType, "damage_amount");
        bind_component_access<Scene>(ecsType, "scene");

        lua.new_usertype<Event::RemoveEntity>("RemoveEntityEvent", sol::no_constructor,
            "entity", &Event::RemoveEntity::entity);
        lua.new_usertype<Event::CollisionDetected>("CollisionDetectedEvent", sol::no_constructor,
            "first", &Event::CollisionDetected::first,
            "second", &Event::CollisionDetected::second,
            "normal", &Event::CollisionDetected::normal,
            "penetration", &Event::CollisionDetected::penetration);
        lua.new_usertype<Event::HitDetected>("HitDetectedEvent", sol::no_constructor,
            "first", &Event::HitDetected::first,
            "second", &Event::HitDetected::second);
        lua.new_usertype<Event>("Event", sol::no_constructor,
            "kind", [](const Event& value) {
                return std::visit([](const auto& event) -> std::string {
                    using T = std::decay_t<decltype(event)>;
                    if constexpr (std::is_same_v<T, Event::RemoveEntity>) return "remove_entity";
                    if constexpr (std::is_same_v<T, Event::CollisionDetected>) return "collision_detected";
                    return "hit_detected";
                }, value.event_data);
            },
            "remove_entity", [](const Event& value) -> std::optional<Event::RemoveEntity> {
                if (const auto* event = std::get_if<Event::RemoveEntity>(&value.event_data)) return *event;
                return std::nullopt;
            },
            "collision_detected", [](const Event& value) -> std::optional<Event::CollisionDetected> {
                if (const auto* event = std::get_if<Event::CollisionDetected>(&value.event_data)) return *event;
                return std::nullopt;
            },
            "hit_detected", [](const Event& value) -> std::optional<Event::HitDetected> {
                if (const auto* event = std::get_if<Event::HitDetected>(&value.event_data)) return *event;
                return std::nullopt;
            });
        lua.new_usertype<EventDispatcher>("EventDispatcher", sol::no_constructor,
            "empty", &EventDispatcher::empty,
            "poll", &EventDispatcher::poll,
            "push_remove_entity", [](EventDispatcher& dispatcher, ecs::Entity entity) {
                dispatcher.push(Event(Event::RemoveEntity{entity}));
            },
            "push_collision_detected", [](EventDispatcher& dispatcher, ecs::Entity first,
                                           ecs::Entity second, const sf::Vector2f& normal, float penetration) {
                dispatcher.push(Event(Event::CollisionDetected{first, second, normal, penetration}));
            },
            "push_hit_detected", [](EventDispatcher& dispatcher, ecs::Entity first, ecs::Entity second) {
                dispatcher.push(Event(Event::HitDetected{first, second}));
            });
        lua.new_usertype<boost::signals2::connection>("EventConnection", sol::no_constructor,
            "connected", &boost::signals2::connection::connected,
            "disconnect", &boost::signals2::connection::disconnect);

        auto eventHandling = lua.new_usertype<utility::EventHandling>("EventHandling", sol::no_constructor);
        eventHandling["on_closed"] = make_event_subscriber<sf::Event::Closed>();
        eventHandling["on_resized"] = make_event_subscriber<sf::Event::Resized>();
        eventHandling["on_focus_lost"] = make_event_subscriber<sf::Event::FocusLost>();
        eventHandling["on_focus_gained"] = make_event_subscriber<sf::Event::FocusGained>();
        eventHandling["on_text_entered"] = make_event_subscriber<sf::Event::TextEntered>();
        eventHandling["on_key_pressed"] = make_event_subscriber<sf::Event::KeyPressed>();
        eventHandling["on_key_released"] = make_event_subscriber<sf::Event::KeyReleased>();
        eventHandling["on_mouse_wheel_scrolled"] = make_event_subscriber<sf::Event::MouseWheelScrolled>();
        eventHandling["on_mouse_button_pressed"] = make_event_subscriber<sf::Event::MouseButtonPressed>();
        eventHandling["on_mouse_button_released"] = make_event_subscriber<sf::Event::MouseButtonReleased>();
        eventHandling["on_mouse_moved"] = make_event_subscriber<sf::Event::MouseMoved>();
        eventHandling["on_mouse_moved_raw"] = make_event_subscriber<sf::Event::MouseMovedRaw>();
        eventHandling["on_mouse_entered"] = make_event_subscriber<sf::Event::MouseEntered>();
        eventHandling["on_mouse_left"] = make_event_subscriber<sf::Event::MouseLeft>();
        eventHandling["on_joystick_button_pressed"] = make_event_subscriber<sf::Event::JoystickButtonPressed>();
        eventHandling["on_joystick_button_released"] = make_event_subscriber<sf::Event::JoystickButtonReleased>();
        eventHandling["on_joystick_moved"] = make_event_subscriber<sf::Event::JoystickMoved>();
        eventHandling["on_joystick_connected"] = make_event_subscriber<sf::Event::JoystickConnected>();
        eventHandling["on_joystick_disconnected"] = make_event_subscriber<sf::Event::JoystickDisconnected>();
        eventHandling["on_touch_began"] = make_event_subscriber<sf::Event::TouchBegan>();
        eventHandling["on_touch_moved"] = make_event_subscriber<sf::Event::TouchMoved>();
        eventHandling["on_touch_ended"] = make_event_subscriber<sf::Event::TouchEnded>();
        eventHandling["on_sensor_changed"] = make_event_subscriber<sf::Event::SensorChanged>();
        eventHandling["on_remove_entity"] = make_event_subscriber<Event::RemoveEntity>();
        eventHandling["on_collision_detected"] = make_event_subscriber<Event::CollisionDetected>();
        eventHandling["on_hit_detected"] = make_event_subscriber<Event::HitDetected>();

        lua.new_usertype<systems::Quadtree>("Quadtree", sol::no_constructor,
            "world_bounds", &systems::Quadtree::worldBounds,
            "insert", [](systems::Quadtree& tree, ecs::Entity entity, const sf::FloatRect& bounds) {
                tree.insert(entity, bounds);
            },
            "remove", [](systems::Quadtree& tree, ecs::Entity entity) { tree.remove(entity); },
            "update", [](systems::Quadtree& tree, ecs::Entity entity, const sf::FloatRect& bounds) {
                tree.update(entity, bounds);
            },
            "query", [](const systems::Quadtree& tree, const sf::FloatRect& bounds) {
                return sol::as_table(tree.query(bounds));
            },
            "query_radius", [](const systems::Quadtree& tree, float radius, const sf::Vector2f& point) {
                return sol::as_table(tree.query(radius, point));
            },
            "query_all", [](const systems::Quadtree& tree) { return sol::as_table(tree.queryAll()); },
            "clear", &systems::Quadtree::clear);
        lua.new_usertype<World>("World", sol::no_constructor,
            "game", sol::property(&World::getGame),
            "ecs", sol::property(&World::getECS),
            "quadtree", sol::property(&World::getQuadtree),
            "event_dispatcher", sol::property(&World::getEventDispatcher),
            "main_window", sol::property(&World::getMainWindow),
            "collision_detector", sol::property(&World::getCollisionDetector),
            "update", &World::update);

        lua.new_usertype<scene_logic::scenes::SceneBase>("Scene", sol::no_constructor,
            "name", sol::property(&scene_logic::scenes::SceneBase::getName),
            "world", sol::property(&scene_logic::scenes::SceneBase::getWorld));
        lua.new_usertype<scene_logic::SceneManager>("SceneManager", sol::no_constructor,
            "change_scene", &scene_logic::SceneManager::changeScene,
            "current_scene", sol::property(&scene_logic::SceneManager::getCurrentScene));

        lua.new_usertype<utility::AssetManager>("AssetManager", sol::no_constructor,
            "load_assets", &utility::AssetManager::loadAssets,
            "get_texture", [](utility::AssetManager& assets, const std::string& name) -> sf::Texture& {
                const auto it = assets.textureMap.find(name);
                if (it == assets.textureMap.end()) throw std::runtime_error("unknown texture asset: " + name);
                return it->second;
            },
            "get_animated_sprite", [](utility::AssetManager& assets, const std::string& name) -> utility::AnimatedSprite& {
                const auto it = assets.spriteMap.find(name);
                if (it == assets.spriteMap.end()) throw std::runtime_error("unknown animated sprite asset: " + name);
                return it->second;
            });

        lua.new_usertype<Game>("Game", sol::no_constructor,
            "main_window", sol::property(&Game::getMainWindow),
            "event_handler", sol::property(&Game::getEventHandler),
            "asset_manager", sol::property(&Game::getAssetManager),
            "scene_manager", sol::property(&Game::getSceneManager),
            "world", sol::property(&Game::getCurrentWorld),
            "request_close", &Game::requestClose,
            "update", &Game::update);

        auto componentTypes = lua.create_named_table("components");
        componentTypes["BehaviorTree"] = lua.get<sol::object>("BehaviorTree");
        componentTypes["BoundingBox"] = lua.get<sol::object>("BoundingBox");
        componentTypes["CollisionShape"] = lua.get<sol::object>("CollisionShape");
        componentTypes["DamageAmount"] = lua.get<sol::object>("DamageAmount");
        componentTypes["Drawable"] = lua.get<sol::object>("DrawableComponent");
        componentTypes["Enemy"] = lua.get<sol::object>("Enemy");
        componentTypes["Hitbox"] = lua.get<sol::object>("Hitbox");
        componentTypes["Player"] = lua.get<sol::object>("Player");
        componentTypes["Projectile"] = lua.get<sol::object>("Projectile");
        componentTypes["Rigidbody"] = lua.get<sol::object>("Rigidbody");
        componentTypes["Scene"] = lua.get<sol::object>("SceneComponent");
        componentTypes["Transform"] = lua.get<sol::object>("Transform");
        componentTypes["TurretMountLayout"] = lua.get<sol::object>("TurretMountLayout");

        auto gameEvents = lua.create_named_table("events");
        gameEvents["Event"] = lua.get<sol::object>("Event");
        gameEvents["RemoveEntity"] = lua.get<sol::object>("RemoveEntityEvent");
        gameEvents["CollisionDetected"] = lua.get<sol::object>("CollisionDetectedEvent");
        gameEvents["HitDetected"] = lua.get<sol::object>("HitDetectedEvent");
        lua["game"] = &game;
    }
}

#endif // METEORSHIP_LUA_GAME_BINDINGS_H
