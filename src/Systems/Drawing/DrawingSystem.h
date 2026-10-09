//
// Created by iformat on 6/24/26.
//

#ifndef METEORSHIP_DRAWINGSYSTEM_H
#define METEORSHIP_DRAWINGSYSTEM_H

#include <SFML/Graphics/RenderWindow.hpp>

#include "Components/Drawable.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"

namespace ms::systems {
    class DrawingSystem : public ecs::System {
        sf::RenderWindow& renderWindow;
    public:
        explicit DrawingSystem(sf::RenderWindow& _renderWindow) : renderWindow(_renderWindow) {};

        void update(float deltaTime) override {
            ecs->view<components::Transform, components::Drawable>().each(
                [this](ecs::Entity const& entity, components::Transform& transform, components::Drawable& drawable) {
                    sf::RenderStates states = sf::RenderStates::Default;
                    states.transform *= transform.getTransform();

                    if (drawable.vertices.has_value()) renderWindow.draw(drawable.vertices.value(), states);
                    if (drawable.sprite.has_value()) renderWindow.draw(drawable.sprite.value(), states);
            });
        }
    };
}
#endif //METEORSHIP_DRAWINGSYSTEM_H
