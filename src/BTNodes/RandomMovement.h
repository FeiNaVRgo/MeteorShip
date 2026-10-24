//
// Created by iformat on 10/24/26.
//

#ifndef METEORSHIP_RANDOMMOVEMENT_H
#define METEORSHIP_RANDOMMOVEMENT_H
#include <behaviortree_cpp/action_node.h>

namespace ms::bt_nodes {
    class RandomMovement : public BT::SyncActionNode {
    public:
        RandomMovement(std::string const& name, BT::NodeConfiguration const& config);
        static BT::PortsList providedPorts();
        BT::NodeStatus tick() override;
    };
}

#endif //METEORSHIP_RANDOMMOVEMENT_H
