#include "CollisionResponseSystem.h"

#include <cmath>
#include <optional>

#include "Components/CollisionShape.h"
#include "Components/Player.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Events/EventSignalFactory.h"
#include "Systems/Physics/CollisionGeometry.h"

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
            !ecs.hasComponent<components::Transform>(second) ||
            !ecs.hasComponent<components::CollisionShape>(first) ||
            !ecs.hasComponent<components::CollisionShape>(second)) {
            return;
        }

        const auto polygonFor = [&ecs](const ecs::Entity entity) {
            const auto& shape = ecs.getComponent<components::CollisionShape>(entity);
            const auto& transform = ecs.getComponent<components::Transform>(entity);
            return collision_geometry::transformPolygon(
                shape.vertices, transform.getTransform());
        };
        const auto firstPolygon = polygonFor(first);
        const auto secondPolygon = polygonFor(second);
        std::optional<collision_geometry::Contact> contact;
        if (collision_geometry::isConvex(firstPolygon) &&
            collision_geometry::isConvex(secondPolygon)) {
            contact = collision_geometry::intersectConvex(firstPolygon, secondPolygon);
        } else {
            contact = collision_geometry::intersect(
                collision_geometry::triangulate(firstPolygon),
                collision_geometry::triangulate(secondPolygon));
        }
        if (!contact || !std::isfinite(contact->penetration) ||
            contact->penetration <= 0.f ||
            !std::isfinite(contact->normal.x) ||
            !std::isfinite(contact->normal.y)) {
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
            contact->normal.x * contact->normal.x +
            contact->normal.y * contact->normal.y;
        if (normalLengthSquared <= 1e-12f) {
            return;
        }
        const auto normal = contact->normal / std::sqrt(normalLengthSquared);
        const auto correction = normal * (contact->penetration / inverseMassSum);
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
