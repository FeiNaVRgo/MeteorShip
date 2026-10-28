//
// Created by iformat on 10/10/26.
//

#ifndef METEORSHIP_PLAYERSYSTEM_H
#define METEORSHIP_PLAYERSYSTEM_H
#include <SFML/Window/Keyboard.hpp>

#include "Components/Player.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "Utility/UtiFuncs.h"

namespace ms::systems {
    class PlayerSystem : public ecs::System {
    public:
        void update(float) override {
            ecs->view<components::Player, components::Transform>()
                .each([](ecs::Entity const&,
                         components::Player&,
                         components::Transform& transform)
                {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                        auto const offset = utility::angleToVector(transform.getRotation()).normalized();
                        transform.move(offset);
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                        transform.rotate(sf::degrees(-0.5f));
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                        transform.rotate(sf::degrees(0.5f));
                    }
                }
            );
        }
    };
}

#endif //METEORSHIP_PLAYERSYSTEM_H