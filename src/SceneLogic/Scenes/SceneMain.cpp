//
// Created by iformat on 10/15/26.
//

#include "SceneMain.h"
#include "Game.h"
#include <utility>

#include "Events/EventSignalFactory.h"
#include "Systems/Physics/MovementSystem.h"
#include "Systems/Player/PlayerSystem.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Utility/UtiFuncs.h"

namespace ms::scene_logic::scenes {
    SceneMain::SceneMain(Game& game, std::string name) : SceneBase(game, std::move(name)) {
        world->registerSystem<systems::SpatialHash>(world->getQuadtree(), world->getEventDispatcher());
        world->registerSystem<systems::DrawingSystem>(game.getMainWindow());
        world->registerSystem<systems::PlayerSystem>(world->getEventDispatcher());
        world->registerSystem<systems::MovementSystem>();
    }

    void SceneMain::init() {
        playerShotConn = utility::EventSignalFactory::createEventSignal<Event::PlayerShot>(
      game.getEventHandler(),
      [this](Event::PlayerShot const& e) { createPlayerBullet(e.player); });

        removeEntityConn = utility::EventSignalFactory::createEventSignal<Event::RemoveEntity>(
            game.getEventHandler(),
            [this](Event::RemoveEntity const& e) { world->getECS().destroyEntity(e.entity); });

        createPlayer();
    }

    void SceneMain::destroy() {
        playerShotConn.disconnect();
        removeEntityConn.disconnect();
    }

    void SceneMain::update(float elapsedTime) {
        world->getQuadtree().draw(game.getMainWindow());
        world->getECS().update(elapsedTime);
    }
    void SceneMain::createPlayerBullet(ecs::Entity player) {
        auto const bullet = world->getECS().createEntity();

        auto const& p_transform = world->getECS().getComponent<components::Transform>(player);

        components::Transform transform{};
        transform.setRotation(p_transform.getRotation());
        transform.setPosition(p_transform.getPosition());
        world->getECS().addComponent<components::Transform>(bullet, transform);

        components::Drawable drawable{};
        drawable.sprite = game.getAssetManager().spriteMap.at("Bullets-spritesheet");
        world->getECS().addComponent<components::Drawable>(bullet, drawable);

        components::Rigidbody rigidbody{};
        auto rect = drawable.sprite.value().sprite.getLocalBounds();
        rect.position = transform.getPosition();
        rigidbody.bounds = rect;

        rigidbody.velocity = utility::angleToVector(transform.getRotation()).normalized();

        world->getECS().addComponent<components::Rigidbody>(bullet, rigidbody);

        world->getQuadtree().insert(bullet, rigidbody.bounds);
    }

    void SceneMain::createPlayer() {
        auto const player = world->getECS().createEntity();

        world->getECS().addComponent<components::Player>(player);
        world->getECS().addComponent<components::ShootingAccess>(player, components::ShootingAccess(1.f));

        components::Transform transform{};
        transform.setRotation(sf::degrees(0));
        transform.setPosition(sf::Vector2f(game.getMainWindow().getSize().x / 2.f, game.getMainWindow().getSize().y / 2.f));
        world->getECS().addComponent<components::Transform>(player, transform);

        components::Drawable drawable{};
        drawable.sprite = game.getAssetManager().spriteMap.at("JetFighter-Var3-Spritesheet");
        world->getECS().addComponent<components::Drawable>(player, drawable);

        components::Rigidbody rigidbody{};
        auto rect = drawable.sprite.value().sprite.getLocalBounds();
        rect.position = transform.getPosition();
        rigidbody.bounds = rect;
        world->getECS().addComponent<components::Rigidbody>(player, rigidbody);

        world->getQuadtree().insert(player, rigidbody.bounds);
    }
}
