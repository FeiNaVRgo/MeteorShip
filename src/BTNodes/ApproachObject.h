//
// Created by iformat on 10/20/26.
//

#ifndef METEORSHIP_APPROACHOBJECT_H
#define METEORSHIP_APPROACHOBJECT_H
#include <behaviortree_cpp/action_node.h>
#include <behaviortree_cpp/bt_factory.h>

namespace ms::bt_nodes {
    class ApproachObject : public BT::SyncActionNode {
    public:
        ApproachObject(const std::string& name) : BT::SyncActionNode(name, {}) {}

        BT::NodeStatus tick() override {
            std::cout << "ApproachObject: " << this->name() << std::endl;
            return BT::NodeStatus::SUCCESS;
        }
    };
}

#endif //METEORSHIP_APPROACHOBJECT_H
