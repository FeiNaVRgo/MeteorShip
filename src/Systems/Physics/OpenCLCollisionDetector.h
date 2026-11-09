#ifndef METEORSHIP_OPENCLCOLLISIONDETECTOR_H
#define METEORSHIP_OPENCLCOLLISIONDETECTOR_H

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <SFML/System/Vector2.hpp>

namespace ms::systems {
    struct SatTrianglePair {
        std::array<sf::Vector2f, 3> first;
        std::array<sf::Vector2f, 3> second;
    };

    struct SatContact {
        bool intersects{};
        sf::Vector2f normal{};
        float penetration{};
    };

    class OpenCLCollisionDetector {
        class Impl;
        std::unique_ptr<Impl> impl;

    public:
        OpenCLCollisionDetector();
        ~OpenCLCollisionDetector();

        OpenCLCollisionDetector(const OpenCLCollisionDetector&) = delete;
        OpenCLCollisionDetector& operator=(const OpenCLCollisionDetector&) = delete;

        [[nodiscard]] std::optional<std::vector<SatContact>> detect(
            const std::vector<SatTrianglePair>& trianglePairs);
    };
}

#endif //METEORSHIP_OPENCLCOLLISIONDETECTOR_H
