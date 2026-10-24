//
// Created by Copilot on 10/24/26.
//

#ifndef METEORSHIP_MOVETOWARDS_H
#define METEORSHIP_MOVETOWARDS_H

#include <behaviortree_cpp/action_node.h>

namespace ms::bt_nodes {
    class MoveTowards : public BT::SyncActionNode {
    public:
        MoveTowards(std::string const& name, BT::NodeConfig const& config);
        static BT::PortsList providedPorts();
        BT::NodeStatus tick() override;
    };
}

#endif // METEORSHIP_MOVETOWARDS_H
