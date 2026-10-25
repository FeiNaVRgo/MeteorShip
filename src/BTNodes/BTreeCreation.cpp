//
// Created by iformat on 10/21/26.
//

#include "BTNodes/BTreeCreation.h"
#include "behaviortree_cpp/xml_parsing.h"
#include "BTContext.h"
#include "Components/Player.h"
#include "Components/Transform.h"
#include "Game.h"

ms::components::BehaviorTree ms::create_tree_for_basic_enemy(Game& game, ecs::Entity entity) {
    auto bb = BT::Blackboard::create(game.getGlobalBlackboard());
    const auto world = game.getSceneManager().getCurrentScene()->getWorld();
    std::optional<ecs::Entity> playerEntity;
    world->getECS().view<components::Player, components::Transform>().each(
        [&playerEntity](ecs::Entity const& player, components::Player const&, components::Transform const&) {
            if (!playerEntity) {
                playerEntity = player;
            }
        });

    bb->set("bt_context", bt_nodes::BTContext{
        .world = world,
        .entity = entity,
        .playerEntity = playerEntity
    });
    bb->set("player_visible", false);

    auto tree = game.getBehaviorTreeFactory().createTreeFromFile("./BTreeXML/normal_enemy.xml", bb);
    return components::BehaviorTree{
        .BTTree = std::move(tree),
        .blackboard = std::move(bb)
    };
}
