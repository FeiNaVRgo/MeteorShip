//
// Created by iformat on 10/25/26.
//

#ifndef METEORSHIP_ROTATETOWARDS_H
#define METEORSHIP_ROTATETOWARDS_H
#include <behaviortree_cpp/action_node.h>

namespace ms::bt_nodes {
    class RotateTowards : public BT::SyncActionNode {
        public:
        RotateTowards(std::string const& name, BT::NodeConfig const& config);
        static BT::PortsList providedPorts();
        BT::NodeStatus tick() override;
    };
}

#endif //METEORSHIP_ROTATETOWARDS_H
