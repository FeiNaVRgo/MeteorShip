//
// Created by iformat on 10/20/26.
//

#include "ApproachObject.h"

#include "Game.h"

namespace ms::bt_nodes {
    ApproachObject::ApproachObject(std::string const& name, BT::NodeConfig const& config) : BT::SyncActionNode(name, config) {}

    BT::PortsList ApproachObject::providedPorts() {
        return { BT::InputPort<Game*>("game") };
    }

    BT::NodeStatus ApproachObject::tick() {
        auto game = getInput<Game*>("game");
        if (!game) {
            return BT::NodeStatus::FAILURE;
        }
        std::cout << game.value()->getSceneManager().getCurrentScene()->getName() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
}
