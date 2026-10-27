//
// Created by iformat on 10/15/26.
//

#include "SceneMain.h"
#include "Game.h"
#include <utility>

#include "BTNodes/BTreeCreation.h"
#include "Components/Enemy.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Behavior/BehaviorSystem.h"
#include "Systems/Physics/MovementSystem.h"
#include "Systems/Player/PlayerSystem.h"
#include "Systems/SpatialHashing/SpatialHash.h"
#include "Systems/Drawing/DrawingSystem.h"
#include "Utility/UtiFuncs.h"

namespace ms::scene_logic::scenes {
    SceneMain::SceneMain(Game& game, std::string name) : SceneBase(game, std::move(name)) {
        world->registerSystem<systems::SpatialHash>(world->getQuadtree(), world->getEventDispatcher());
        world->registerSystem<systems::DrawingSystem>(game.getMainWindow());
        world->registerSystem<systems::PlayerSystem>(world->getEventDispatcher());
        world->registerSystem<systems::MovementSystem>();
        world->registerSystem<systems::BehaviorSystem>();

        const auto& spellTexture = game.getAssetManager().textureMap.at("Bullets-spritesheet");
        sf::Sprite fireIcon(spellTexture);
        fireIcon.setTextureRect(sf::IntRect({0, 0}, {8, 8}));
        sf::Sprite frostIcon(spellTexture);
        frostIcon.setTextureRect(sf::IntRect({8, 0}, {8, 8}));
        inventory.storeSpell(inventory::Spell("Fire", std::move(fireIcon)));
        inventory.storeSpell(inventory::Spell("Frost", std::move(frostIcon)));

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

        /*
        auto label = std::make_unique<ui::UINode>(ui::widgets::LabelWidget{"Root Node"});
        //label->size = {50, 15};
        auto btn   = std::make_unique<ui::UINode>(ui::widgets::ButtonWidget{"Click Me", []{ std::cout << "button clicked\n"; }},
                                             sf::Vector2f{50.f, 10.f});
        btn->size = {50, 15};
        auto sldr  = std::make_unique<ui::UINode>(ui::widgets::SliderWidget{0.7f},
                                             sf::Vector2f{50.f, 100.f});
        sldr->size = {50, 15};
        auto nested = std::make_unique<ui::UINode>(ui::widgets::LabelWidget{"Nested Label"},
                                              sf::Vector2f{40.f, 30.f});
        //nested->size = {50, 15};
        btn->addChild(std::move(nested));

        ui.root->addChild(std::move(label));
        ui.root->addChild(std::move(btn));
        ui.root->addChild(std::move(sldr));
        */
    }

    void SceneMain::init() {
        playerShotConn = utility::EventSignalFactory::createEventSignal<Event::PlayerShot>(
      game.getEventHandler(),
      [this](Event::PlayerShot const& e) { createPlayerBullet(e.player); });

        removeEntityConn = utility::EventSignalFactory::createEventSignal<Event::RemoveEntity>(
            game.getEventHandler(),
            [this](Event::RemoveEntity const& e) { world->getECS().destroyEntity(e.entity); });

        createPlayer();
        createEnemy();
    }

    void SceneMain::destroy() {
        playerShotConn.disconnect();
        removeEntityConn.disconnect();
    }

    void SceneMain::update(float elapsedTime) {
        world->getQuadtree().draw(game.getMainWindow());
        world->getECS().update(elapsedTime);
    }
    //TODO: tag system for bullets so they know what they can hit and what they cant
    //lowk every entity should know what they can collide with or damage
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
        rect.position = transform.getPosition();
        rigidbody.bounds = rect;
        world->getECS().addComponent<components::Rigidbody>(enemy, rigidbody);

        world->getQuadtree().insert(enemy, rigidbody.bounds);


    }
}
