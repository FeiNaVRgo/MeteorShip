//
// Created by iformat on 10/25/26.
//

#include "RotateTowards.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "BTContext.h"
#include "Components/Player.h"
#include "Components/Transform.h"
#include "ECS/World.h"

namespace ms::bt_nodes {
    RotateTowards::RotateTowards(std::string const& name, BT::NodeConfig const& config)
        : BT::SyncActionNode(name, config) {}

    BT::PortsList RotateTowards::providedPorts() {
        return {
            BT::InputPort<BTContext>("bt_context"),
            BT::InputPort<float>("speed")
        };
    }

    BT::NodeStatus RotateTowards::tick() {
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
        if (!ecs.isEntityAlive(entity) || !ecs.hasComponent<components::Transform>(entity)) {
            return BT::NodeStatus::FAILURE;
        }

        auto& transform = ecs.getComponent<components::Transform>(entity);
        const auto position = transform.getPosition();
        const auto playerEntity = btContext.value().playerEntity;
        if (!playerEntity ||
            *playerEntity == entity ||
            !ecs.isEntityAlive(*playerEntity) ||
            !ecs.hasComponent<components::Player>(*playerEntity) ||
            !ecs.hasComponent<components::Transform>(*playerEntity)) {
            return BT::NodeStatus::FAILURE;
        }

        const auto playerPosition = ecs.getComponent<components::Transform>(*playerEntity).getPosition();
        const auto direction = playerPosition - position;
        if (!std::isfinite(direction.x) || !std::isfinite(direction.y)) {
            return BT::NodeStatus::FAILURE;
        }
        if (direction.x == 0.f && direction.y == 0.f) {
            return BT::NodeStatus::SUCCESS;
        }

        const float targetDegrees = std::atan2(direction.x, -direction.y) * 180.f / std::numbers::pi_v<float>;
        const float currentDegrees = transform.getRotation().asDegrees();
        if (!std::isfinite(currentDegrees)) {
            return BT::NodeStatus::FAILURE;
        }

        const float difference = std::remainder(targetDegrees - currentDegrees, 360.f);
        const float turn = std::clamp(difference, -speed.value(), speed.value());
        transform.rotate(sf::degrees(turn));
        return BT::NodeStatus::SUCCESS;
    }
}
