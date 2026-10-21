//
// Created by iformat on 10/21/26.
//

#include "BTNodes/BTreeCreation.h"
#include "behaviortree_cpp/xml_parsing.h"
#
#include "ApproachObject.h"
#include "Game.h"

ms::components::BehaviorTree ms::create_tree_for_basic_enemy(Game& game) {
    auto tree = game.getBehaviorTreeFactory().createTreeFromFile("./BTreeXML/normal_enemy.xml", game.getGlobalBlackboard());
    return components::BehaviorTree{(std::move(tree)), game.getGlobalBlackboard()};
}
