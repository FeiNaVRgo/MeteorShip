#include "MoveForward.h"

#include <cmath>

#include "BTContext.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Utility/UtiFuncs.h"

namespace ms::bt_nodes {
    MoveForward::MoveForward(std::string const& name, BT::NodeConfig const& config)
        : BT::SyncActionNode(name, config) {}

    BT::PortsList MoveForward::providedPorts() {
        return {
            BT::InputPort<BTContext>("bt_context"),
            BT::InputPort<float>("speed")
        };
    }

    BT::NodeStatus MoveForward::tick() {
        const auto btContext = getInput<BTContext>("bt_context");
        const auto speed = getInput<float>("speed");
        if (!btContext || !speed || !std::isfinite(speed.value()) || speed.value() < 0.f) {
            return BT::NodeStatus::FAILURE;
        }

        const auto world = btContext.value().world.lock();
        if (!world) {
            return BT::NodeStatus::FAILURE;
        }

        auto& ecs = world->getECS();
        const auto entity = btContext.value().entity;
        if (!ecs.isEntityAlive(entity) ||
            !ecs.hasComponent<components::Transform>(entity) ||
            !ecs.hasComponent<components::Rigidbody>(entity)) {
            return BT::NodeStatus::FAILURE;
        }

        const auto rotation = ecs.getComponent<components::Transform>(entity).getRotation();
        if (!std::isfinite(rotation.asDegrees())) {
            return BT::NodeStatus::FAILURE;
        }

        ecs.getComponent<components::Rigidbody>(entity).velocity =
            utility::angleToVector(rotation) * speed.value();
        return BT::NodeStatus::SUCCESS;
    }
}
