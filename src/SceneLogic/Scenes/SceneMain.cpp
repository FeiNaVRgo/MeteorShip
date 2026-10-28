//
// Created by iformat on 10/15/26.
//

#include "SceneMain.h"
#include "Game.h"
#include "Utility/UtiFuncs.h"
#include <fstream>
#include <sstream>
#include <utility>

#include <boost/json.hpp>

#include "BTNodes/BTreeCreation.h"
#include "Components/Enemy.h"
#include "Components/Projectile.h"
#include "Components/TurretMountLayout.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Behavior/BehaviorSystem.h"
#include "Systems/Physics/MovementSystem.h"
#include "Systems/Player/PlayerSystem.h"
#include "Systems/Player/TurretSystem.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Systems/Drawing/DrawingSystem.h"
#include "Utility/UtiFuncs.h"

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
        world->registerSystem<systems::MovementSystem>();
        world->registerSystem<systems::BehaviorSystem>();
        world->registerSystem<systems::SpatialHash>(world->getQuadtree(), world->getEventDispatcher());
        world->registerSystem<systems::TurretSystem>(
            inventory, game.getMainWindow(), world->getQuadtree());
        world->registerSystem<systems::DrawingSystem>(game.getMainWindow());

        inventory.bindECS(world->getECS());

        const auto& fireProjectile = game.getAssetManager().spriteMap.at("Fire-projectile");
        const auto& frostProjectile = game.getAssetManager().spriteMap.at("Frost-projectile");
        sf::Sprite fireIcon = fireProjectile.sprite;
        sf::Sprite frostIcon = frostProjectile.sprite;
        inventory::ProjectileStats fireStats;
        fireStats.damage = {{components::DamageType::Projectile, 1.5f}};
        inventory.storeSpell(inventory::Spell(
            "Fire", inventory::ProjectileSpell{std::move(fireStats)}, std::move(fireIcon),
            fireProjectile,
            sf::Color(255, 120, 45)));
        inventory::ProjectileStats frostStats;
        frostStats.speed = 10;
        inventory.storeSpell(inventory::Spell(
            "Frost", inventory::ProjectileSpell{std::move(frostStats)}, std::move(frostIcon),
            frostProjectile,
            sf::Color(100, 190, 255)));

        const auto& turretTexture = game.getAssetManager().textureMap.at("JetFighter-Var3-Spritesheet");
        sf::Sprite turretIcon(turretTexture);
        turretIcon.setTextureRect(sf::IntRect({0, 0}, {32, 32}));
        sf::Sprite turretIcon2(turretTexture);
        turretIcon2.setTextureRect(sf::IntRect({0, 0}, {32, 32}));
        inventory::Turret pulseTurret(
            "Pulse Turret", std::move(turretIcon), 5);
        inventory::Turret pulseTurret2(
            "Pulse Turret", std::move(turretIcon2), 2);
        inventory.storeTurret(std::move(pulseTurret));
        inventory.storeTurret(std::move(pulseTurret2));

        ui.root = std::make_unique<ui::UINode>();
        inventory.buildUI(*ui.root);
    }

    void SceneMain::init() {
        removeEntityConn = utility::EventSignalFactory::createEventSignal<Event::RemoveEntity>(
            game.getEventHandler(),
            [this](Event::RemoveEntity const& e) { world->getECS().destroyEntity(e.entity); });

        createPlayer();
        createEnemy();
    }

    void SceneMain::destroy() {
        removeEntityConn.disconnect();
    }

    void SceneMain::update(float elapsedTime) {
        world->getQuadtree().draw(game.getMainWindow());
        world->getECS().update(elapsedTime);
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
        inventory.bindPlayerEntity(player);

        components::Rigidbody rigidbody{};
        auto rect = drawable.sprite.value().sprite.getLocalBounds();
        rigidbody.bounds = utility::centeredRect(transform.getPosition(), rect.size);
        world->getECS().addComponent<components::Rigidbody>(player, rigidbody);

        world->getQuadtree().insert(player, rigidbody.bounds);
    }

    void SceneMain::createEnemy() {
        auto const enemy = world->getECS().createEntity();

        world->getECS().addComponent<components::Enemy>(enemy);
        world->getECS().addComponent<components::BehaviorTree>(enemy, create_tree_for_basic_enemy(game, enemy));

        components::Transform transform{};
        transform.setRotation(sf::degrees(0));
        transform.setPosition(sf::Vector2f(game.getMainWindow().getSize().x / 2.f, game.getMainWindow().getSize().y / 2.f));
        world->getECS().addComponent<components::Transform>(enemy, transform);

        components::Drawable drawable{};
        drawable.sprite = game.getAssetManager().spriteMap.at("JetFighter-Var2-Spritesheet");
        world->getECS().addComponent<components::Drawable>(enemy, drawable);

        components::Rigidbody rigidbody{};
        auto rect = drawable.sprite.value().sprite.getLocalBounds();
        rigidbody.bounds = utility::centeredRect(transform.getPosition(), rect.size);
        world->getECS().addComponent<components::Rigidbody>(enemy, rigidbody);

        world->getQuadtree().insert(enemy, rigidbody.bounds);


    }
}
