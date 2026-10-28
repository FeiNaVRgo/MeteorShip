//
// Created by ricardo on 18.10.2026.
//

#ifndef METEORSHIP_UTIFUNCS_H
#define METEORSHIP_UTIFUNCS_H
#include <cmath>
#include <numbers>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

namespace ms::utility {
    inline sf::Vector2f angleToVector(const sf::Angle angle) {
        return { std::sin(angle.asRadians()), -std::cos(angle.asRadians()) };
    }

    inline sf::Vector2f directionFromAngle(const sf::Angle angle) {
        return {std::cos(angle.asRadians()), std::sin(angle.asRadians())};
    }

    inline sf::Angle angleFromDirection(const sf::Vector2f direction) {
        return sf::radians(std::atan2(direction.y, direction.x));
    }

    inline sf::Angle rotationFromDirection(const sf::Vector2f direction) {
        return sf::radians(std::atan2(direction.x, -direction.y));
    }

    inline float squaredLength(const sf::Vector2f vector) {
        return vector.x * vector.x + vector.y * vector.y;
    }

    inline float squaredDistance(const sf::Vector2f first, const sf::Vector2f second) {
        return squaredLength(second - first);
    }

    inline sf::FloatRect centeredRect(const sf::Vector2f center, const sf::Vector2f size) {
        return {center - size / 2.f, size};
    }
}

#endif //METEORSHIP_UTIFUNCS_H
