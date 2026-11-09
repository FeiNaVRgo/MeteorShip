#include "CollisionResponseSystem.h"

#include <cmath>

#include "Components/Player.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Events/EventSignalFactory.h"

namespace ms::systems {
    CollisionResponseSystem::CollisionResponseSystem(utility::EventHandling& eventHandler) {
        collisionDetectedConn = utility::EventSignalFactory::createEventSignal<Event::CollisionDetected>(
            eventHandler,
            [this](const Event::CollisionDetected& event) { handleCollisionDetected(event); });
    }

    void CollisionResponseSystem::handleCollisionDetected(const Event::CollisionDetected& event) {
        auto& ecs = getWorld().getECS();
        const auto first = event.first;
        const auto second = event.second;
        if (!ecs.isEntityAlive(first) || !ecs.isEntityAlive(second) ||
            !ecs.hasComponent<components::Transform>(first) ||
            !ecs.hasComponent<components::Transform>(second)) {
            return;
        }

        if (!std::isfinite(event.penetration) || event.penetration <= 0.f ||
            !std::isfinite(event.normal.x) || !std::isfinite(event.normal.y)) {
            return;
        }

        const bool firstHasBody = ecs.hasComponent<components::Rigidbody>(first);
        const bool secondHasBody = ecs.hasComponent<components::Rigidbody>(second);
        const bool firstIsPlayer = ecs.hasComponent<components::Player>(first);
        const bool secondIsPlayer = ecs.hasComponent<components::Player>(second);
        if (firstHasBody && firstIsPlayer) {
            ecs.getComponent<components::Rigidbody>(first).velocity = {};
        }
        if (secondHasBody && secondIsPlayer) {
            ecs.getComponent<components::Rigidbody>(second).velocity = {};
        }

        const float firstInverseMass = firstHasBody && !firstIsPlayer ? 1.f : 0.f;
        const float secondInverseMass = secondHasBody && !secondIsPlayer ? 1.f : 0.f;
        const float inverseMassSum = firstInverseMass + secondInverseMass;
        if (inverseMassSum == 0.f) {
            return;
        }

        const float normalLengthSquared =
            event.normal.x * event.normal.x +
            event.normal.y * event.normal.y;
        if (normalLengthSquared <= 1e-12f) {
            return;
        }
        const auto normal = event.normal / std::sqrt(normalLengthSquared);
        const auto correction = normal * (event.penetration / inverseMassSum);
        const auto firstCorrection = -correction * firstInverseMass;
        const auto secondCorrection = correction * secondInverseMass;
        ecs.getComponent<components::Transform>(first).move(firstCorrection);
        ecs.getComponent<components::Transform>(second).move(secondCorrection);

        const auto firstVelocity = firstInverseMass > 0.f
                                       ? ecs.getComponent<components::Rigidbody>(first).velocity
                                       : sf::Vector2f{};
        const auto secondVelocity = secondInverseMass > 0.f
                                        ? ecs.getComponent<components::Rigidbody>(second).velocity
                                        : sf::Vector2f{};
        const auto relativeVelocity = secondVelocity - firstVelocity;
        const float normalVelocity =
            relativeVelocity.x * normal.x + relativeVelocity.y * normal.y;
        if (normalVelocity >= 0.f) {
            return;
        }

        const float impulseMagnitude = -normalVelocity / inverseMassSum;
        const auto impulse = normal * impulseMagnitude;
        if (firstHasBody) {
            ecs.getComponent<components::Rigidbody>(first).velocity -=
                impulse * firstInverseMass;
        }
        if (secondHasBody) {
            ecs.getComponent<components::Rigidbody>(second).velocity +=
                impulse * secondInverseMass;
        }
    }
}
