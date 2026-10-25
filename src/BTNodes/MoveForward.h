//
// Created by Copilot on 10/25/26.
//

#ifndef METEORSHIP_MOVEFORWARD_H
#define METEORSHIP_MOVEFORWARD_H

#include <behaviortree_cpp/action_node.h>

namespace ms::bt_nodes {
    class MoveForward : public BT::SyncActionNode {
    public:
        MoveForward(std::string const& name, BT::NodeConfig const& config);
        static BT::PortsList providedPorts();
        BT::NodeStatus tick() override;
    };
}

#endif // METEORSHIP_MOVEFORWARD_H
