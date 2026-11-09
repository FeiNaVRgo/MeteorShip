#ifndef METEORSHIP_COLLISIONSHAPE_H
#define METEORSHIP_COLLISIONSHAPE_H

#include <array>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace ms::components {
    struct CollisionShape {
        using Triangle = std::array<sf::Vector2f, 3>;

        CollisionShape() = default;
        explicit CollisionShape(std::vector<sf::Vector2f> vertices);

        [[nodiscard]] const std::vector<sf::Vector2f>& getVertices() const {
            return vertices;
        }

        [[nodiscard]] const std::vector<Triangle>& getTriangles() const {
            return triangles;
        }

    private:
        std::vector<sf::Vector2f> vertices;
        std::vector<Triangle> triangles;
    };
}

#endif //METEORSHIP_COLLISIONSHAPE_H
