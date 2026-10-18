//
// Created by ricardo on 18.10.2026.
//

#ifndef METEORSHIP_UTIFUNCS_H
#define METEORSHIP_UTIFUNCS_H
#include <cmath>
#include <SFML/System/Vector2.hpp>

namespace ms::utility {
    inline sf::Vector2f angleToVector(sf::Angle const angle) {
        return { std::sin(angle.asRadians()), -std::cos(angle.asRadians()) };
    }
}

#endif //METEORSHIP_UTIFUNCS_H
