//
// Created by iformat on 10/15/26.
//

#include "SceneMain.h"
#include "Game.h"
#include "Utility/UtiFuncs.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <boost/json.hpp>

#include "BTNodes/BTreeCreation.h"
#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Enemy.h"
#include "Components/Hitbox.h"
#include "Components/Player.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/TurretMountLayout.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Behavior/BehaviorSystem.h"
#include "Systems/Physics/CollisionSystem.h"
#include "Systems/Physics/CollisionResponseSystem.h"
#include "Systems/Physics/HitSystem.h"
#include "Systems/Physics/HitResponseSystem.h"
#include "Systems/Physics/MovementSystem.h"
#include "Systems/Projectiles/ProjectileSystem.h"
#include "Systems/Player/PlayerSystem.h"
#include "Systems/Player/TurretSystem.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Systems/Drawing/DrawingSystem.h"
#include "Utility/CustomEntityLoader.h"

namespace ms::scene_logic::scenes {
    namespace {
        std::vector<sf::Vector2f> loadTurretMountOffsets() {
            constexpr auto configPath = "./assets/ships/player_ship.json";
            std::ifstream configFile(configPath);
            if (!configFile) {
                throw std::runtime_error("Failed to open ship layout: " + std::string(configPath));
            }

            std::stringstream contents;
            contents << configFile.rdbuf();
            const auto config = boost::json::parse(contents.str());
            const auto& mounts = config.as_object().at("turret_mounts").as_array();
            std::vector<sf::Vector2f> offsets;
            offsets.reserve(mounts.size());
            const auto coordinate = [](const boost::json::value& value) {
                return static_cast<float>(
                    value.is_double() ? value.as_double() : static_cast<double>(value.as_int64()));
            };
            for (const auto& mount : mounts) {
                const auto& object = mount.as_object();
                offsets.emplace_back(
                    coordinate(object.at("x")),
                    coordinate(object.at("y"))
                );
            }
            if (offsets.size() < inventory::DefaultTurretMountCount) {
                throw std::runtime_error("Ship layout must define at least one hardpoint per turret mount.");
            }
            return offsets;
        }

    }

    SceneMain::SceneMain(Game& game, std::string name) : SceneBase(game, std::move(name)) {
        world->registerSystem<systems::PlayerSystem>();
        world->registerSystem<systems::ProjectileSystem>();
        world->registerSystem<systems::MovementSystem>();
        world->registerSystem<systems::BehaviorSystem>();
        world->registerSystem<systems::SpatialHash>();
        auto& collisionSystem = world->registerSystem<systems::CollisionSystem>();
        auto& collisionResponse = world->registerSystem<systems::CollisionResponseSystem>();
        collisionSystem.setCollisionResponseSystem(&collisionResponse);
        world->registerSystem<systems::HitSystem>();
        world->registerSystem<systems::HitResponseSystem>(game.getEventHandler());
        world->registerSystem<systems::TurretSystem>(inventory);
        world->registerSystem<systems::DrawingSystem>();

        ui.root = std::make_unique<ui::UINode>();

        inventory.buildUI(*ui.root);
        utility::CustomEntityLoader loader{this->game};
        std::unordered_map<std::string, inventory::Spell> spellsById;
        for (auto& definition : loader.loadSpells()) {
            spellsById.emplace(definition.id, definition.spell);
            if (!inventory.storeSpell(std::move(definition.spell))) {
                throw std::runtime_error("The custom spell inventory exceeds its available slots.");
            }
        }
        std::size_t turretIndex = 0;
        for (auto& definition : loader.loadTurrets()) {
            if (turretIndex >= inventory.turretMountCount()) {
                throw std::runtime_error("There are more custom turrets than available player mounts.");
            }
            if (!inventory.setTurret(turretIndex, std::move(definition.turret))) {
                throw std::runtime_error("Failed to mount custom turret: " + definition.id);
            }
            for (std::size_t slot = 0; slot < definition.spells.size(); ++slot) {
                const auto spell = spellsById.find(definition.spells[slot]);
                if (spell == spellsById.end()) {
                    throw std::runtime_error("Turret '" + definition.id + "' references unknown spell '" +
                                             definition.spells[slot] + "'.");
                }
                if (slot >= inventory.turretSpellSlotCount(turretIndex) ||
                    !inventory.setSpell({inventory::SlotCollection::TurretSpell, slot, turretIndex},
                                        spell->second)) {
                    throw std::runtime_error("Turret '" + definition.id +
                                             "' does not have enough spell slots for its deck.");
                }
            }
            ++turretIndex;
        }
        enemyDefinitions = loader.loadEnemies();
    }

    void SceneMain::init() {
        removeEntityConn = utility::EventSignalFactory::createEventSignal<Event::RemoveEntity>(
            game.getEventHandler(),
            [this](Event::RemoveEntity const& e) { world->getECS().destroyEntity(e.entity); });
        createPlayer();
        for (const auto& definition : enemyDefinitions) createEnemy(definition);
    }

    void SceneMain::destroy() {
        removeEntityConn.disconnect();
    }

    void SceneMain::update(float elapsedTime) {
        world->getQuadtree().draw(game.getMainWindow());

        constexpr float fixedTimeStep = 1.f / 60.f;
        constexpr float maximumFrameTime = 0.25f;

        simulationAccumulator += std::min(elapsedTime, maximumFrameTime);

        while (simulationAccumulator >= fixedTimeStep) {
            world->getECS().update(fixedTimeStep);
            simulationAccumulator -= fixedTimeStep;
        }

        world->getECS().render();

        while (const auto event = world->getEventDispatcher().poll()) {
            game.getEventHandler().handleEvent(event);
        }
    }
    void SceneMain::createPlayer() {
        auto const player = world->getECS().createEntity();

        world->getECS().addComponent<components::Player>(player);
        world->getECS().addComponent<components::TurretMountLayout>(
            player, components::TurretMountLayout{loadTurretMountOffsets()});

        components::Transform transform{};
        transform.setRotation(sf::degrees(0));
        transform.setPosition(sf::Vector2f(game.getMainWindow().getSize().x / 2.f, game.getMainWindow().getSize().y / 2.f));
        world->getECS().addComponent<components::Transform>(player, transform);

        components::Drawable drawable{};
        drawable.sprite = game.getAssetManager().spriteMap.at("JetFighter-Var3-Spritesheet");
        world->getECS().addComponent<components::Drawable>(player, drawable);
        components::Rigidbody rigidbody{};
        auto rect = drawable.sprite.value().sprite.getLocalBounds();
        const auto halfSize = rect.size / 2.f;
        const sf::FloatRect localBounds(-halfSize, rect.size);
        auto& boundingBox = world->getECS().addComponent<components::BoundingBox>(
            player, components::BoundingBox{
                utility::centeredRect(transform.getPosition(), rect.size)});
        world->getECS().addComponent<components::CollisionShape>(
            player, components::CollisionShape{{
                {0.f, -halfSize.y},
                {2.f, -15.f},
                {3.f, -5.f},
                {5.f, -4.f},
                {6.f, -2.f},
                {7.f, 0.f},
                {9.f, 2.f},
                {11.f, 4.f},
                {13.f, 6.f},
                {14.f, 7.f},
                {14.f, 14.f},
                {8.f, 15.f},
                {4.f, halfSize.y},
                {-4.f, halfSize.y},
                {-8.f, 15.f},
                {-14.f, 14.f},
                {-14.f, 7.f},
                {-13.f, 6.f},
                {-11.f, 4.f},
                {-9.f, 2.f},
                {-7.f, 0.f},
                {-6.f, -2.f},
                {-5.f, -4.f},
                {-3.f, -5.f},
                {-2.f, -15.f}
            }});
        world->getECS().addComponent<components::Hitbox>(
            player, components::Hitbox{{localBounds}});
        world->getECS().addComponent<components::Rigidbody>(player, rigidbody);

        world->getQuadtree().insert(player, boundingBox.bounds);
    }

    void SceneMain::createEnemy() {
        if (enemyDefinitions.empty()) {
            throw std::runtime_error("No custom enemy definitions were loaded.");
        }
        createEnemy(enemyDefinitions.front());
    }

    void SceneMain::createEnemy(const utility::CustomEnemyDefinition& definition) {
        auto const enemy = world->getECS().createEntity();
        std::size_t enemyIndex = 0;
        world->getECS().view<components::Enemy>().each(
            [&enemyIndex](const ecs::Entity, const components::Enemy&) {
                ++enemyIndex;
            });

        world->getECS().addComponent<components::Enemy>(enemy);
        world->getECS().addComponent<components::BehaviorTree>(
            enemy, utility::CustomEntityLoader::createEnemyBehaviorTree(
                       game, world, enemy, definition.behaviorTree, definition.parameters));

        components::Transform transform{};
        transform.setRotation(sf::degrees(0));
        const auto windowSize = game.getMainWindow().getSize();
        constexpr std::size_t spawnColumns = 8;
        constexpr float spawnSpacing = 48.f;
        const auto column = enemyIndex % spawnColumns;
        const auto row = enemyIndex / spawnColumns;
        transform.setPosition({
            windowSize.x * 0.75f +
                (static_cast<float>(column) - (spawnColumns - 1) / 2.f) * spawnSpacing,
            windowSize.y / 2.f +
                (static_cast<float>(row) - 3.f) * spawnSpacing
        });
        world->getECS().addComponent<components::Transform>(enemy, transform);

        components::Drawable drawable{};
        drawable.sprite = game.getAssetManager().spriteMap.at(definition.sprite);
        world->getECS().addComponent<components::Drawable>(enemy, drawable);

        components::Rigidbody rigidbody{};
        auto rect = drawable.sprite.value().sprite.getLocalBounds();
        const auto halfSize = rect.size / 2.f;
        auto& boundingBox = world->getECS().addComponent<components::BoundingBox>(
            enemy, components::BoundingBox{
                utility::centeredRect(transform.getPosition(), rect.size)});
        world->getECS().addComponent<components::CollisionShape>(
            enemy, components::CollisionShape{{
                {0.f, -halfSize.y},
                {2.f, -15.f},
                {3.f, -5.f},
                {5.f, -4.f},
                {6.f, -2.f},
                {7.f, 0.f},
                {9.f, 2.f},
                {11.f, 4.f},
                {13.f, 6.f},
                {14.f, 7.f},
                {14.f, 14.f},
                {8.f, 15.f},
                {4.f, halfSize.y},
                {-4.f, halfSize.y},
                {-8.f, 15.f},
                {-14.f, 14.f},
                {-14.f, 7.f},
                {-13.f, 6.f},
                {-11.f, 4.f},
                {-9.f, 2.f},
                {-7.f, 0.f},
                {-6.f, -2.f},
                {-5.f, -4.f},
                {-3.f, -5.f},
                {-2.f, -15.f}
            }});
        world->getECS().addComponent<components::Hitbox>(
            enemy, components::Hitbox{{
                sf::FloatRect(
                    {-halfSize.x * 0.25f, -halfSize.y},
                    {halfSize.x * 0.5f, rect.size.y}),
                sf::FloatRect(
                    {-halfSize.x, -halfSize.y * 0.1f},
                    {halfSize.x * 0.35f, halfSize.y * 0.35f}),
                sf::FloatRect(
                    {-halfSize.x, halfSize.y * 0.25f},
                    {halfSize.x * 0.35f, halfSize.y * 0.35f}),
                sf::FloatRect(
                    {halfSize.x * 0.65f, -halfSize.y * 0.1f},
                    {halfSize.x * 0.35f, halfSize.y * 0.35f}),
                sf::FloatRect(
                    {halfSize.x * 0.65f, halfSize.y * 0.25f},
                    {halfSize.x * 0.35f, halfSize.y * 0.35f})
            }});
        world->getECS().addComponent<components::Rigidbody>(enemy, rigidbody);

        world->getQuadtree().insert(enemy, boundingBox.bounds);


    }
}
