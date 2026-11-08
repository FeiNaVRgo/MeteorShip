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
#include "Components/Player.h"
#include "Components/Projectile.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/TurretMountLayout.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Behavior/BehaviorSystem.h"
#include "Systems/Physics/CollisionSystem.h"
#include "Systems/Physics/MovementSystem.h"
#include "Systems/Projectiles/ProjectileSpawner.h"
#include "Systems/Projectiles/ProjectileSystem.h"
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

        bool sameFaction(ecs::ECS& ecs, const ecs::Entity first, const ecs::Entity second) {
            return (ecs.hasComponent<components::Player>(first) &&
                    ecs.hasComponent<components::Player>(second)) ||
                   (ecs.hasComponent<components::Enemy>(first) &&
                    ecs.hasComponent<components::Enemy>(second));
        }

        bool canProjectileHit(
            ecs::ECS& ecs,
            const components::Projectile& projectile,
            const ecs::Entity targetEntity) {
            const bool ownerAlive = ecs.isEntityAlive(projectile.owner);
            if (ownerAlive && targetEntity == projectile.owner) {
                return false;
            }
            if (projectile.friendlyFire || !ownerAlive) {
                return true;
            }

            return !sameFaction(ecs, projectile.owner, targetEntity);
        }

        sf::Vector2f projectileHitNormal(
            const sf::FloatRect& projectileBounds,
            const sf::FloatRect& targetBounds) {
            const auto overlap = projectileBounds.findIntersection(targetBounds);
            if (!overlap) {
                return {};
            }

            const float projectileCenterX =
                projectileBounds.position.x + projectileBounds.size.x / 2.f;
            const float targetCenterX =
                targetBounds.position.x + targetBounds.size.x / 2.f;
            const float projectileCenterY =
                projectileBounds.position.y + projectileBounds.size.y / 2.f;
            const float targetCenterY =
                targetBounds.position.y + targetBounds.size.y / 2.f;

            if (overlap->size.x < overlap->size.y) {
                return {projectileCenterX < targetCenterX ? -1.f : 1.f, 0.f};
            }
            return {0.f, projectileCenterY < targetCenterY ? -1.f : 1.f};
        }

        void handleProjectileHit(
            ecs::ECS& ecs,
            const ecs::Entity projectileEntity,
            const ecs::Entity targetEntity,
            const sf::Vector2f hitNormal) {
            auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);
            if (projectile.piercing && !projectile.hitEntities.insert(targetEntity).second) {
                return;
            }

            const auto modifierHooks = projectile.modifierHooks;
            const bool piercing = projectile.piercing;
            const auto triggerType = projectile.triggerType;
            inventory::ModifierContext context{
                .entityManager = ecs,
                .projectileEntity = projectileEntity,
                .targetEntity = targetEntity,
                .hitNormal = hitNormal,
                .deltaTime = 0.f
            };
            const bool collisionTrigger = triggerType == inventory::TriggerType::Collision;
            if (collisionTrigger) {
                systems::releaseProjectilePayload(ecs, projectileEntity);
            }

            for (const auto& hook : modifierHooks) {
                if (hook.onCollision) {
                    hook.onCollision(context);
                }
            }

            if ((!collisionTrigger && piercing) || !ecs.isEntityAlive(projectileEntity) ||
                !ecs.hasComponent<components::Projectile>(projectileEntity)) {
                return;
            }

            for (const auto& hook : modifierHooks) {
                if (hook.onDeath) {
                    hook.onDeath(context);
                }
            }
            if (ecs.isEntityAlive(projectileEntity)) {
                ecs.destroyEntity(projectileEntity);
            }
        }
    }

    SceneMain::SceneMain(Game& game, std::string name) : SceneBase(game, std::move(name)) {
        world->registerSystem<systems::PlayerSystem>();
        world->registerSystem<systems::ProjectileSystem>();
        world->registerSystem<systems::MovementSystem>();
        world->registerSystem<systems::BehaviorSystem>();
        world->registerSystem<systems::SpatialHash>();
        world->registerSystem<systems::CollisionSystem>();
        world->registerSystem<systems::TurretSystem>(inventory);
        world->registerSystem<systems::DrawingSystem>();

        ui.root = std::make_unique<ui::UINode>();

        inventory.buildUI(*ui.root);
        //for testing ig
        inventory.storeTurret(inventory::Turret{
            "turret numero jeden",
            this->game.getAssetManager().spriteMap.at("JetFighter-Var3-Spritesheet").sprite,
            7
        });

        auto& assetMgr = this->game.getAssetManager();
        std::optional<utility::AnimatedSprite> bulletSprite;
        std::optional<sf::Sprite> bulletIcon;
        auto bulletIt = assetMgr.spriteMap.find("Fire-projectile");
        if (bulletIt != assetMgr.spriteMap.end()) {
            bulletSprite = bulletIt->second;
            bulletIcon = bulletIt->second.sprite;
        }

        auto deck = inventory::SpellFactory::makeExampleDeck();
        for (auto& spell : deck) {
            if (spell.isProjectile() && bulletSprite.has_value()) {
                spell.setProjectileSprite(*bulletSprite);
                spell.setIcon(*bulletIcon);
            }
            inventory.storeSpell(std::move(spell));
        }
    }

    void SceneMain::init() {
        removeEntityConn = utility::EventSignalFactory::createEventSignal<Event::RemoveEntity>(
            game.getEventHandler(),
            [this](Event::RemoveEntity const& e) { world->getECS().destroyEntity(e.entity); });
        collisionDetectedConn =
            utility::EventSignalFactory::createEventSignal<Event::CollisionDetected>(
                game.getEventHandler(),
                [this](const Event::CollisionDetected& event) { handleCollisionDetected(event); });

        createPlayer();
        createEnemy();
    }

    void SceneMain::destroy() {
        removeEntityConn.disconnect();
        collisionDetectedConn.disconnect();
    }

    void SceneMain::handleCollisionDetected(const Event::CollisionDetected& event) {
        auto& ecs = world->getECS();
        const auto first = event.first;
        const auto second = event.second;
        if (!ecs.isEntityAlive(first) || !ecs.isEntityAlive(second) ||
            !ecs.hasComponent<components::Rigidbody>(first) ||
            !ecs.hasComponent<components::Rigidbody>(second) ||
            !ecs.hasComponent<components::Transform>(first) ||
            !ecs.hasComponent<components::Transform>(second)) {
            return;
        }

        auto& firstBody = ecs.getComponent<components::Rigidbody>(first);
        auto& secondBody = ecs.getComponent<components::Rigidbody>(second);
        const auto overlap = firstBody.bounds.findIntersection(secondBody.bounds);
        if (!overlap) {
            return;
        }

        const bool firstIsProjectile = ecs.hasComponent<components::Projectile>(first);
        const bool secondIsProjectile = ecs.hasComponent<components::Projectile>(second);
        if (firstIsProjectile || secondIsProjectile) {
            if (firstIsProjectile && secondIsProjectile) {
                return;
            }

            const auto projectileEntity = firstIsProjectile ? first : second;
            const auto targetEntity = firstIsProjectile ? second : first;
            const auto& projectile = ecs.getComponent<components::Projectile>(projectileEntity);
            if (!canProjectileHit(ecs, projectile, targetEntity)) {
                return;
            }

            const auto hitNormal = projectileHitNormal(
                ecs.getComponent<components::Rigidbody>(projectileEntity).bounds,
                ecs.getComponent<components::Rigidbody>(targetEntity).bounds);
            handleProjectileHit(ecs, projectileEntity, targetEntity, hitNormal);
            return;
        }

        sf::Vector2f firstCorrection{};
        if (overlap->size.x < overlap->size.y) {
            const float firstCenter = firstBody.bounds.position.x + firstBody.bounds.size.x / 2.f;
            const float secondCenter = secondBody.bounds.position.x + secondBody.bounds.size.x / 2.f;
            const float direction = firstCenter == secondCenter
                                        ? (first < second ? -1.f : 1.f)
                                        : (firstCenter < secondCenter ? -1.f : 1.f);
            firstCorrection.x = direction * overlap->size.x / 2.f;
        } else {
            const float firstCenter = firstBody.bounds.position.y + firstBody.bounds.size.y / 2.f;
            const float secondCenter = secondBody.bounds.position.y + secondBody.bounds.size.y / 2.f;
            const float direction = firstCenter == secondCenter
                                        ? (first < second ? -1.f : 1.f)
                                        : (firstCenter < secondCenter ? -1.f : 1.f);
            firstCorrection.y = direction * overlap->size.y / 2.f;
        }

        const sf::Vector2f secondCorrection = -firstCorrection;
        ecs.getComponent<components::Transform>(first).move(firstCorrection);
        ecs.getComponent<components::Transform>(second).move(secondCorrection);
        firstBody.bounds.position += firstCorrection;
        secondBody.bounds.position += secondCorrection;

        const auto removeNormalVelocity = [](components::Rigidbody& body, sf::Vector2f correction) {
            if (correction.x != 0.f) {
                body.velocity.x = 0.f;
            } else {
                body.velocity.y = 0.f;
            }
        };
        removeNormalVelocity(firstBody, firstCorrection);
        removeNormalVelocity(secondBody, secondCorrection);

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
