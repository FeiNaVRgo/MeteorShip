//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_TRANSFORM_H
#define METEORSHIP_TRANSFORM_H
#include <SFML/System/Vector2.hpp>

namespace ms::components {
    struct Transform {
        sf::Vector2f position;
        sf::Vector2f rotation;
        sf::Vector2f scale;
    };
}

#endif //METEORSHIP_TRANSFORM_H
