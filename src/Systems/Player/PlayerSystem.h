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
        void update(float deltaTime) override {
            ecs->view<components::Player, components::Transform>()
                .each([deltaTime](ecs::Entity const&,
                                  components::Player&,
                                  components::Transform& transform)
                {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                        auto const offset = utility::angleToVector(transform.getRotation()).normalized();
                        transform.move(offset * deltaTime * 50.f);
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                        transform.rotate(sf::degrees(-1.f) * deltaTime * 20.f);
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                        transform.rotate(sf::degrees(1.f) * deltaTime * 20.f);
                    }
                }
            );
        }
    };
}

#endif //METEORSHIP_PLAYERSYSTEM_H