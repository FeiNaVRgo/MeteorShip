#ifndef METEORSHIP_TURRETMOUNTLAYOUT_H
#define METEORSHIP_TURRETMOUNTLAYOUT_H

#include <vector>

#include <SFML/System/Vector2.hpp>

namespace ms::components {
    struct TurretMountLayout {
        std::vector<sf::Vector2f> topLeftOffsets;
    };
}

#endif //METEORSHIP_TURRETMOUNTLAYOUT_H
