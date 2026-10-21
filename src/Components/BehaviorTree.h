//
// Created by iformat on 10/20/26.
//

#ifndef METEORSHIP_BEHAVIORTREE_H
#define METEORSHIP_BEHAVIORTREE_H
#include <behaviortree_cpp/bt_factory.h>

namespace ms::components {
    struct BehaviorTree {
        BT::Tree BTTree; //chai tea
        BT::Blackboard::Ptr blackboard;
    };
}


#endif //METEORSHIP_BEHAVIORTREE_H
