//
// Created by iformat on 10/23/26.
//

#ifndef METEORSHIP_ISPLAYERVISIBLE_H
#define METEORSHIP_ISPLAYERVISIBLE_H
#include <behaviortree_cpp/condition_node.h>

#include "Game.h"
#include "Components/Player.h"

namespace ms::bt_nodes {
    class IsPlayerVisible : public BT::ConditionNode {
    public:
        IsPlayerVisible(std::string const& name, BT::NodeConfiguration const& config);
        static BT::PortsList providedPorts();
        BT::NodeStatus tick() override;
    };
}

#endif //METEORSHIP_ISPLAYERVISIBLE_H
