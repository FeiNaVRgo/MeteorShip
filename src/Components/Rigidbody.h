//
// Created by iformat on 6/4/26.
//

#ifndef METEORSHIP_RIGIDBODY_H
#define METEORSHIP_RIGIDBODY_H
#include <SFML/System/Vector2.hpp>

namespace ms::components {
    struct Rigidbody {
        sf::Vector2f velocity;
        sf::Vector2f acceleration;
    };
}

#endif //METEORSHIP_RIGIDBODY_H
