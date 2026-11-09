#include "CollisionShape.h"

#include <utility>

#include "Systems/Physics/CollisionGeometry.h"

namespace ms::components {
    CollisionShape::CollisionShape(std::vector<sf::Vector2f> vertices)
        : vertices(std::move(vertices)) {
        const auto polygonTriangles =
            systems::collision_geometry::triangulate(this->vertices);
        triangles.reserve(polygonTriangles.size());
        for (const auto& triangle : polygonTriangles) {
            triangles.push_back({triangle[0], triangle[1], triangle[2]});
        }
    }
}
