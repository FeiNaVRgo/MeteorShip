//
// Created by iformat on 10/21/26.
//

#include "BTNodes/BTreeCreation.h"
#include "behaviortree_cpp/xml_parsing.h"
#
#include "ApproachObject.h"
#include "Game.h"

ms::components::BehaviorTree ms::create_tree_for_basic_enemy(Game& game, ecs::Entity entity) {
    auto bb = BT::Blackboard::create(game.getGlobalBlackboard());
    //lowk something like a simple struct for BTContext would be cool :3
    
    bb->set("enemy", entity);
    bb->set("player_visible", false);
    //works so i can now pass event arguments

    auto tree = game.getBehaviorTreeFactory().createTreeFromFile("./BTreeXML/normal_enemy.xml", bb);
    return components::BehaviorTree{(std::move(tree)), std::move(bb)};
}
