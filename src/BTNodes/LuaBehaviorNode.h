#ifndef METEORSHIP_LUA_BEHAVIOR_NODE_H
#define METEORSHIP_LUA_BEHAVIOR_NODE_H

#include <behaviortree_cpp/action_node.h>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/condition_node.h>

namespace ms {
    class Game;
}

namespace ms::bt_nodes {
    void registerLuaBehaviorNodes(BT::BehaviorTreeFactory& factory, Game& game);
}

#endif // METEORSHIP_LUA_BEHAVIOR_NODE_H
