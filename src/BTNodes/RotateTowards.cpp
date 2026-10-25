//
// Created by iformat on 10/25/26.
//

#include "RotateTowards.h"

#include "BTContext.h"

namespace ms::bt_nodes {
    RotateTowards::RotateTowards(std::string const& name, BT::NodeConfig const& config)
        : BT::SyncActionNode(name, config) {}

    BT::PortsList RotateTowards::providedPorts() {
        return {
            BT::InputPort<BTContext>("bt_context"),
            BT::InputPort<float>("speed")
        };
    }

    BT::NodeStatus RotateTowards::tick() {
        
    }
}
