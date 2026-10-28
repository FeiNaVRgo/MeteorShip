#include "MoveTowards.h"

#include <cmath>

#include "BTContext.h"
#include "Components/Player.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Utility/UtiFuncs.h"

namespace ms::bt_nodes {
    MoveTowards::MoveTowards(std::string const& name, BT::NodeConfig const& config)
        : BT::SyncActionNode(name, config) {}

    BT::PortsList MoveTowards::providedPorts() {
        return {
            BT::InputPort<BTContext>("bt_context"),
            BT::InputPort<float>("speed")
        };
    }

    BT::NodeStatus MoveTowards::tick() {
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

        auto& rigidbody = ecs.getComponent<components::Rigidbody>(entity);
        const auto playerEntity = btContext.value().playerEntity;
        if (!playerEntity ||
            *playerEntity == entity ||
            !ecs.isEntityAlive(*playerEntity) ||
            !ecs.hasComponent<components::Player>(*playerEntity) ||
            !ecs.hasComponent<components::Transform>(*playerEntity)) {
            rigidbody.velocity = {};
            return BT::NodeStatus::FAILURE;
        }

        const auto playerPosition = ecs.getComponent<components::Transform>(*playerEntity).getPosition();
        const auto direction = playerPosition - ecs.getComponent<components::Transform>(entity).getPosition();
        const float distanceSquared = utility::squaredLength(direction);
        if (distanceSquared <= 0.001f * 0.001f) {
            rigidbody.velocity = {};
            return BT::NodeStatus::SUCCESS;
        }

        rigidbody.velocity = direction / std::sqrt(distanceSquared) * speed.value();
        return BT::NodeStatus::SUCCESS;
    }
}
