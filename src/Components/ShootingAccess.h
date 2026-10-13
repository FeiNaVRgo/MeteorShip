//
// Created by iformat on 10/12/26.
//

#ifndef METEORSHIP_SHOOTINGACCESS_H
#define METEORSHIP_SHOOTINGACCESS_H
#include <SFML/System/Clock.hpp>

namespace ms::components {
    struct ShootingAccess {
        sf::Clock shootingClock;
        float shootingInterval;

        explicit ShootingAccess(float const shootingInterval) : shootingInterval(shootingInterval) {}
    };
}

#endif //METEORSHIP_SHOOTINGACCESS_H
