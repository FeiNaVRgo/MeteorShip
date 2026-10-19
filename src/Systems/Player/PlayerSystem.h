//
// Created by iformat on 10/10/26.
//

#ifndef METEORSHIP_PLAYERSYSTEM_H
#define METEORSHIP_PLAYERSYSTEM_H
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "Components/Drawable.h"
#include "Components/Player.h"
#include "Components/Transform.h"
#include "Components/ShootingAccess.h"
#include "ECS/ECS.h"
#include "Utility/UtiFuncs.h"

namespace ms::systems {
    class PlayerSystem : public ecs::System {
        EventDispatcher& eventDispatcher;
        public:
        explicit  PlayerSystem(EventDispatcher& eventDispatcher) : eventDispatcher(eventDispatcher) {}

        void update(float deltaTime) override {
            ecs->view<components::Player, components::Transform, components::ShootingAccess, components::Rigidbody, components::Drawable>()
                .each([this](ecs::Entity const& entity,
                                  components::Player& player,
                                  components::Transform& transform,
                                  components::ShootingAccess& shootingAccess,
                                  components::Rigidbody& rigidbody,
                                  components::Drawable& drawable)
                {
                    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                        if (shootingAccess.shootingClock.getElapsedTime().asSeconds() >= shootingAccess.shootingInterval) {
                            eventDispatcher.push(Event(Event::PlayerShot(entity)));
                            shootingAccess.shootingClock.restart();
                        }
                    }

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