//
// Created by iformat on 10/24/26.
//

#include "RandomMovement.h"

#include "BTContext.h"
#include "Game.h"
#include "ECS/ECS.h"

namespace ms::bt_nodes {
    RandomMovement::RandomMovement(std::string const &name, BT::NodeConfiguration const &config) : BT::SyncActionNode(name, config) {}
    BT::PortsList RandomMovement::providedPorts() {
        return { BT::InputPort<Game*>("game"),
                    BT::InputPort<BTContext>("bt_context") };
    }

    BT::NodeStatus RandomMovement::tick() {
        std::cout << this->name() << std::endl;
        auto game = getInput<Game*>("game");
        auto btContext = getInput<BTContext>("bt_context");
        if (!game || !btContext) return BT::NodeStatus::FAILURE;

        return BT::NodeStatus::SUCCESS;
    }
}
