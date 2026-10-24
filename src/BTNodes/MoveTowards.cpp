#include "MoveTowards.h"

#include <cmath>

#include "BTContext.h"
#include "Components/Player.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"

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

        bool playerFound = false;
        sf::Vector2f playerPosition{};
        ecs.view<components::Player, components::Transform>().each(
            [&playerFound, &playerPosition, entity](
                ecs::Entity const& playerEntity,
                components::Player const&,
                components::Transform const& transform) {
                if (playerEntity != entity && !playerFound) {
                    playerPosition = transform.getPosition();
                    playerFound = true;
                }
            });

        auto& rigidbody = ecs.getComponent<components::Rigidbody>(entity);
        if (!playerFound) {
            rigidbody.velocity = {};
            return BT::NodeStatus::FAILURE;
        }

        const auto direction = playerPosition - ecs.getComponent<components::Transform>(entity).getPosition();
        const float distance = direction.length();
        if (distance <= 0.001f) {
            rigidbody.velocity = {};
            return BT::NodeStatus::SUCCESS;
        }

        rigidbody.velocity = direction / distance * speed.value();
        return BT::NodeStatus::SUCCESS;
    }
}
