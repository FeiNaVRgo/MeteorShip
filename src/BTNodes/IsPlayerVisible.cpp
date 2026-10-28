//
// Created by iformat on 10/23/26.
//

#include "IsPlayerVisible.h"

#include <cmath>

#include "BTContext.h"
#include "Components/Player.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Utility/UtiFuncs.h"

namespace ms::bt_nodes {
    IsPlayerVisible::IsPlayerVisible(std::string const &name, BT::NodeConfiguration const &config): BT::ConditionNode(name, config) {}

    BT::PortsList IsPlayerVisible::providedPorts() {
        return {
            BT::InputPort<Game*>("game"),
            BT::InputPort<BTContext>("bt_context"),
            BT::InputPort<float>("sight_range"),
            BT::OutputPort<bool>("player_visible")
        };
    }

    BT::NodeStatus IsPlayerVisible::tick() {
        auto btContext = getInput<BTContext>("bt_context");
        auto sightRange = getInput<float>("sight_range");

        if (!btContext || !sightRange || !std::isfinite(sightRange.value()) || sightRange.value() < 0.f) {
            return BT::NodeStatus::FAILURE;
        }

        const auto world = btContext.value().world.lock();
        if (!world) {
            return BT::NodeStatus::FAILURE;
        }

        auto& ecs = world->getECS();
        const auto enemy = btContext.value().entity;
        if (!ecs.isEntityAlive(enemy) || !ecs.hasComponent<components::Transform>(enemy)) {
            return BT::NodeStatus::FAILURE;
        }

        const auto playerEntity = btContext.value().playerEntity;
        if (!playerEntity ||
            *playerEntity == enemy ||
            !ecs.isEntityAlive(*playerEntity) ||
            !ecs.hasComponent<components::Player>(*playerEntity) ||
            !ecs.hasComponent<components::Transform>(*playerEntity)) {
            setOutput("player_visible", false);
            return BT::NodeStatus::FAILURE;
        }

        const auto enemyPosition = ecs.getComponent<components::Transform>(enemy).getPosition();
        const auto playerPosition = ecs.getComponent<components::Transform>(*playerEntity).getPosition();
        const float sightRangeSquared = sightRange.value() * sightRange.value();
        const bool visible =
            utility::squaredDistance(enemyPosition, playerPosition) <= sightRangeSquared;

        setOutput("player_visible", visible);
        return visible ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
}
