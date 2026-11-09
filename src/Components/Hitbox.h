#ifndef METEORSHIP_HITBOX_H
#define METEORSHIP_HITBOX_H

#include <SFML/Graphics/Rect.hpp>
#include <vector>

namespace ms::components {
    struct Hitbox {
        std::vector<sf::FloatRect> rectangles;
    };
}

#endif //METEORSHIP_HITBOX_H
