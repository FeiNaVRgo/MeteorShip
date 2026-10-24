//
// Created by iformat on 10/23/26.
//

#include "IsPlayerVisible.h"

namespace ms::bt_nodes {
    IsPlayerVisible::IsPlayerVisible(std::string const &name, BT::NodeConfiguration const &config): BT::ConditionNode(name, config) {}

    BT::PortsList IsPlayerVisible::providedPorts() {
        return {
            BT::InputPort<Game*>("game"),
            BT::InputPort<ecs::Entity>("entity"),
            BT::OutputPort<bool>("player_visible")
        };
    }

    BT::NodeStatus IsPlayerVisible::tick() {
        std::cout << "is player visible" << std::endl;
        auto game = getInput<Game*>("game");
        auto entity = getInput<ecs::Entity>("entity");

        if (!game || !entity) return BT::NodeStatus::FAILURE;
        auto& ecs = game.value()->getSceneManager().getCurrentScene()->getWorld()->getECS();

        bool visible = false;
        //i need to put actual logic here
        visible = rand() % 2;

        setOutput("player_visible", visible);
        return visible ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
}
