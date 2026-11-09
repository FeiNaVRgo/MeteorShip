#include "CollisionSystem.h"

#include <algorithm>
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
        std::vector<ecs::Entity> polygonEntities;
        std::vector<std::vector<sf::Vector2f>> polygons;
        std::unordered_map<ecs::Entity, sf::FloatRect> entityBounds;
        std::unordered_map<ecs::Entity, collision_geometry::Polygon> entityPolygons;
        entityManager.view<
            components::CollisionShape, components::BoundingBox, components::Transform>()
            .each([&polygonEntities, &polygons, &entityBounds, &entityPolygons](
                const ecs::Entity entity,
                const components::CollisionShape& shape,
                const components::BoundingBox& boundingBox,
                const components::Transform& transform) {
                polygonEntities.push_back(entity);
                auto polygon = collision_geometry::transformPolygon(
                    shape.vertices, transform.getTransform());
                polygons.push_back(polygon);
                entityBounds.emplace(entity, boundingBox.bounds);
                entityPolygons.emplace(entity, std::move(polygon));
            });

        const auto deviceTriangulations =
            world.getCollisionDetector().triangulate(polygons);
        std::unordered_map<ecs::Entity, std::vector<collision_geometry::Polygon>> shapes;
        for (std::size_t polygonIndex = 0;
             polygonIndex < polygonEntities.size(); ++polygonIndex) {
            std::vector<collision_geometry::Polygon> triangles;
            if (deviceTriangulations &&
                !(*deviceTriangulations)[polygonIndex].empty()) {
                triangles.reserve((*deviceTriangulations)[polygonIndex].size());
                for (const auto& triangle : (*deviceTriangulations)[polygonIndex]) {
                    triangles.emplace_back(triangle.begin(), triangle.end());
                }
            } else if (polygons[polygonIndex].size() >= 3) {
                triangles = collision_geometry::triangulate(polygons[polygonIndex]);
            }
            shapes.emplace(polygonEntities[polygonIndex], std::move(triangles));
        }

        std::vector<std::pair<ecs::Entity, ecs::Entity> > candidatePairs;
        for (const auto& [entity, bounds] : entityBounds) {
            if (shapes.at(entity).empty()) {
                continue;
            }
            for (const auto candidate: quadtree.queryPotential(bounds)) {
                if (candidate == entity || !shapes.contains(candidate) ||
                    shapes.at(candidate).empty()) {
                    continue;
                }
                candidatePairs.emplace_back(
                    std::min(entity, candidate), std::max(entity, candidate));
            }
        }
        std::ranges::sort(candidatePairs);
        candidatePairs.erase(std::ranges::unique(candidatePairs).begin(),
                             candidatePairs.end());

        std::vector<std::optional<collision_geometry::Contact>> contacts(candidatePairs.size());
        std::vector<SatTrianglePair> trianglePairs;
        std::vector<std::size_t> trianglePairOwners;
        for (std::size_t pairIndex = 0; pairIndex < candidatePairs.size(); ++pairIndex) {
            const auto [first, second] = candidatePairs[pairIndex];
            const auto& firstPolygon = entityPolygons.at(first);
            const auto& secondPolygon = entityPolygons.at(second);
            if (collision_geometry::isConvex(firstPolygon) &&
                collision_geometry::isConvex(secondPolygon)) {
                contacts[pairIndex] = collision_geometry::intersectConvex(
                    firstPolygon, secondPolygon);
                continue;
            }

            const auto& firstTriangles = shapes.at(first);
            const auto& secondTriangles = shapes.at(second);
            for (const auto& firstTriangle : firstTriangles) {
                for (const auto& secondTriangle : secondTriangles) {
                    trianglePairs.push_back({
                        .first = {
                            firstTriangle[0], firstTriangle[1], firstTriangle[2]
                        },
                        .second = {
                            secondTriangle[0], secondTriangle[1], secondTriangle[2]
                        }
                    });
                    trianglePairOwners.push_back(pairIndex);
                }
            }
        }
        const auto openCLContacts = world.getCollisionDetector().detect(trianglePairs);
        //const auto openCLContacts = world.getCollisionDetector().detect(trianglePairs);
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
                const collision_geometry::Polygon firstTriangle(
                    pair.first.begin(), pair.first.end());
                const collision_geometry::Polygon secondTriangle(
                    pair.second.begin(), pair.second.end());
                contact = collision_geometry::intersectConvex(
                    firstTriangle, secondTriangle);
            }
            auto& bestContact = contacts[trianglePairOwners[index]];
            if (contact && (!bestContact ||
                            contact->penetration > bestContact->penetration)) {
                bestContact = contact;
            }
        }

        collisions.clear();
        for (std::size_t index = 0; index < candidatePairs.size(); ++index) {
            if (!contacts[index]) {
                continue;
            }
            const auto [first, second] = candidatePairs[index];
            collisions.emplace_back(first, second);
            eventDispatcher.push(Event::CollisionDetected{
                .first = first,
                .second = second,
                .normal = contacts[index]->normal,
                .penetration = contacts[index]->penetration
            });
        }
    }
}
