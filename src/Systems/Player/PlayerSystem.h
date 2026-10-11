//
// Created by iformat on 10/10/26.
//

#ifndef METEORSHIP_PLAYERSYSTEM_H
#define METEORSHIP_PLAYERSYSTEM_H
#include <cmath>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "Components/Drawable.h"
#include "Components/Player.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"

namespace ms::systems {
    class PlayerSystem : public ecs::System {
        public:
        void update(float deltaTime) override {
            ecs->view<components::Player, components::Transform>().each(
                [deltaTime](ecs::Entity const& entity, components::Player& player ,components::Transform& transform)
                {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                        auto const offset = sf::Vector2f
                        (
                            std::sin(transform.getRotation().asRadians()),
                            -std::cos(transform.getRotation().asRadians())
                        ).normalized();
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