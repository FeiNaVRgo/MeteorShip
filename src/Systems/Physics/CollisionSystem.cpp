#include "CollisionSystem.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Systems/Physics/CollisionGeometry.h"
#include "Systems/Physics/OpenCLCollisionDetector.h"

namespace ms::systems {
    void CollisionSystem::update(float) {
        auto& world = getWorld();
        auto& entityManager = world.getECS();
        auto& quadtree = world.getQuadtree();
        auto& eventDispatcher = world.getEventDispatcher();

        struct ShapeData {
            ecs::Entity entity;
            sf::FloatRect bounds;
            const components::CollisionShape* shape;
        };
        std::vector<ShapeData> shapes;
        std::unordered_map<ecs::Entity, std::size_t> shapeIndices;
        entityManager.view<
            components::CollisionShape, components::BoundingBox, components::Transform>()
            .each([&shapes, &shapeIndices](
                const ecs::Entity entity,
                const components::CollisionShape& shape,
                const components::BoundingBox& boundingBox,
                const components::Transform&) {
                shapeIndices.emplace(entity, shapes.size());
                shapes.push_back({entity, boundingBox.bounds, &shape});
            });

        std::vector<std::pair<ecs::Entity, ecs::Entity>> candidatePairs;
        for (const auto& shape : shapes) {
            if (shape.shape->getTriangles().empty()) {
                continue;
            }
            for (const auto candidate : quadtree.queryPotential(shape.bounds)) {
                const auto candidateIndex = shapeIndices.find(candidate);
                if (candidate == shape.entity || candidateIndex == shapeIndices.end() ||
                    shapes[candidateIndex->second].shape->getTriangles().empty()) {
                    continue;
                }
                candidatePairs.emplace_back(
                    std::min(shape.entity, candidate), std::max(shape.entity, candidate));
            }
        }
        std::ranges::sort(candidatePairs);
        candidatePairs.erase(
            std::ranges::unique(candidatePairs).begin(), candidatePairs.end());

        std::vector<SatTrianglePair> trianglePairs;
        std::vector<std::size_t> trianglePairOwners;
        std::vector<std::vector<components::CollisionShape::Triangle>>
            transformedTriangles(shapes.size());

        //function for transforming
        const auto transformShape = [&](const std::size_t shapeIndex) {
            const auto& shape = shapes[shapeIndex];
            const auto& transform =
                entityManager.getComponent<components::Transform>(shape.entity).getTransform();
            auto& transformed = transformedTriangles[shapeIndex];
            transformed.reserve(shape.shape->getTriangles().size());
            for (const auto& triangle : shape.shape->getTriangles()) {
                components::CollisionShape::Triangle worldTriangle;
                for (std::size_t vertex = 0; vertex < 3; ++vertex) {
                    worldTriangle[vertex] = transform.transformPoint(triangle[vertex]);
                }
                transformed.push_back(worldTriangle);
            }
        };

        //transforming all triangles
        for (std::size_t shapeIndex = 0; shapeIndex < shapes.size(); ++shapeIndex) {
            transformShape(shapeIndex);
        }

        //making triangle pairs
        for (std::size_t pairIndex = 0; pairIndex < candidatePairs.size(); ++pairIndex) {
            const auto [first, second] = candidatePairs[pairIndex];
            const auto firstShapeIndex = shapeIndices.at(first);
            const auto secondShapeIndex = shapeIndices.at(second);

            for (const auto& firstTriangle : transformedTriangles[firstShapeIndex]) {
                for (const auto& secondTriangle : transformedTriangles[secondShapeIndex]) {
                    trianglePairs.push_back({firstTriangle, secondTriangle});
                    trianglePairOwners.push_back(pairIndex);
                }
            }
        }

        //looking for best contacts from triangles
        const auto openCLContacts = world.getCollisionDetector().detect(trianglePairs);
        std::vector<std::optional<collision_geometry::Contact>> contacts(candidatePairs.size());
        for (std::size_t index = 0; index < trianglePairs.size(); ++index) {
            std::optional<collision_geometry::Contact> contact;
            if (openCLContacts) {
                const auto& result = (*openCLContacts)[index];
                if (result.intersects) {
                    contact = collision_geometry::Contact{
                        result.normal, result.penetration};
                }
            } else {
                const auto& pair = trianglePairs[index];
                contact = collision_geometry::intersectConvex(pair.first, pair.second);
            }
            auto& bestContact = contacts[trianglePairOwners[index]];
            if (contact && (!bestContact ||
                            contact->penetration > bestContact->penetration)) {
                bestContact = contact;
            }
        }

        //resolving the collision
        for (std::size_t index = 0; index < candidatePairs.size(); ++index) {
            if (!contacts[index]) {
                continue;
            }
            const auto [first, second] = candidatePairs[index];
            eventDispatcher.push(Event::CollisionDetected{
                .first = first,
                .second = second,
                .normal = contacts[index]->normal,
                .penetration = contacts[index]->penetration
            });
        }
    }
}
