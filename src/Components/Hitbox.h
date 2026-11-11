#ifndef METEORSHIP_HITBOX_H
#define METEORSHIP_HITBOX_H

#include <cmath>
#include <SFML/Graphics/Rect.hpp>
#include <vector>

namespace ms::components {
    struct OBB2D {
        sf::Vector2f center;
        sf::Vector2f extents;
        sf::Vector2f axes[2];

        OBB2D(const sf::FloatRect& rectangle)
            : center(rectangle.position + rectangle.size / 2.f),
              extents(rectangle.size / 2.f),
              axes{{1.f, 0.f}, {0.f, 1.f}} {}

        OBB2D(const sf::Vector2f center, const sf::Vector2f extents, const sf::Angle angle)
            : center(center),
              extents(extents)
        {
            updateAxes(angle);
        }

        void updateAxes(const sf::Angle angle) {
            const float rad = angle.asRadians();
            auto sinA = std::sin(rad);
            auto cosA = std::cos(rad);

            axes[0] = {cosA, sinA};
            axes[1] = {-sinA, cosA};
        }
    };

    struct Hitbox {
        std::vector<OBB2D> rectangles;
    };
}

#endif //METEORSHIP_HITBOX_H
