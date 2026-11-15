#include "CollisionSystem.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Transform.h"
#include "ECS/World.h"
#include "Systems/Physics/CollisionGeometry.h"
#include "Systems/Physics/CollisionResponseSystem.h"
#include "Systems/Physics/OpenCLCollisionDetector.h"

namespace ms::systems {
    namespace {
        struct TransformedTriangle {
            components::CollisionShape::Triangle vertices;
            sf::FloatRect bounds;
        };

        sf::FloatRect getBounds(
            const components::CollisionShape::Triangle& triangle) {
            float minX = triangle[0].x;
            float maxX = triangle[0].x;
            float minY = triangle[0].y;
            float maxY = triangle[0].y;
            for (std::size_t vertex = 1; vertex < triangle.size(); ++vertex) {
                minX = std::min(minX, triangle[vertex].x);
                maxX = std::max(maxX, triangle[vertex].x);
                minY = std::min(minY, triangle[vertex].y);
                maxY = std::max(maxY, triangle[vertex].y);
            }
            return {{minX, minY}, {maxX - minX, maxY - minY}};
        }
    }

    void CollisionSystem::update(float) {
        auto& world = getWorld();
        auto& entityManager = world.getECS();
        auto& quadtree = world.getQuadtree();
        if (!collisionResponseSystem) {
            throw std::logic_error("CollisionSystem requires a CollisionResponseSystem");
        }

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

        std::vector<std::vector<TransformedTriangle>>
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
                transformed.push_back({worldTriangle, getBounds(worldTriangle)});
            }
        };

        constexpr std::size_t solverIterations = 8;
        for (std::size_t iteration = 0; iteration < solverIterations; ++iteration) {
            for (std::size_t shapeIndex = 0; shapeIndex < shapes.size(); ++shapeIndex) {
                transformedTriangles[shapeIndex].clear();
                transformShape(shapeIndex);
            }

            std::vector<std::optional<collision_geometry::Contact>> contacts(candidatePairs.size());
            bool foundContact = false;
            for (std::size_t pairIndex = 0; pairIndex < candidatePairs.size(); ++pairIndex) {
                const auto [first, second] = candidatePairs[pairIndex];
                const auto& firstTriangles = transformedTriangles[shapeIndices.at(first)];
                const auto& secondTriangles = transformedTriangles[shapeIndices.at(second)];
                for (const auto& firstTriangle : firstTriangles) {
                    for (const auto& secondTriangle : secondTriangles) {
                        if (!firstTriangle.bounds.findIntersection(secondTriangle.bounds)) {
                            continue;
                        }
                        const auto contact = collision_geometry::intersectConvex(
                            firstTriangle.vertices, secondTriangle.vertices);
                        auto& bestContact = contacts[pairIndex];
                        if (contact && (!bestContact ||
                                        contact->penetration > bestContact->penetration)) {
                            bestContact = contact;
                            foundContact = true;
                        }
                    }
                }
            }

            if (!foundContact) {
                break;
            }

            bool correctedPosition = false;
            for (std::size_t index = 0; index < candidatePairs.size(); ++index) {
                if (!contacts[index]) {
                    continue;
                }
                const auto [first, second] = candidatePairs[index];
                correctedPosition |= collisionResponseSystem->resolveCollision(
                    Event::CollisionDetected{
                        .first = first,
                        .second = second,
                        .normal = contacts[index]->normal,
                        .penetration = contacts[index]->penetration
                    });
            }
            if (!correctedPosition) {
                break;
            }
        }
    }

    void CollisionSystem::setCollisionResponseSystem(CollisionResponseSystem *pcollisionResponseSystem) {
        this->collisionResponseSystem = pcollisionResponseSystem;
    }
}
