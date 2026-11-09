#ifndef METEORSHIP_COLLISIONSHAPE_H
#define METEORSHIP_COLLISIONSHAPE_H

#include <SFML/System/Vector2.hpp>
#include <vector>

namespace ms::components {
    struct CollisionShape {
        // Vertices of a simple closed polygon; concave polygons are triangulated for SAT.
        std::vector<sf::Vector2f> vertices;
    };
}

#endif //METEORSHIP_COLLISIONSHAPE_H
