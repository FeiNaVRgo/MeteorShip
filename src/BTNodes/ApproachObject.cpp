//
// Created by iformat on 10/20/26.
//

#include "ApproachObject.h"

#include "Game.h"

namespace ms::bt_nodes {
    ApproachObject::ApproachObject(std::string const& name, BT::NodeConfig const& config) : BT::SyncActionNode(name, config) {}

    BT::PortsList ApproachObject::providedPorts() {
        return { BT::InputPort<Game*>("game"), BT::InputPort<ecs::Entity>("entity") };
    }

    BT::NodeStatus ApproachObject::tick() {
        auto game = getInput<Game*>("game");
        auto entity = getInput<ecs::Entity>("entity");
        if (!entity) {
            return BT::NodeStatus::FAILURE;
        }
        std::cout << entity.value() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
}
