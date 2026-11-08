//
// Created by iformat on 6/24/26.
//

#ifndef METEORSHIP_DRAWINGSYSTEM_H
#define METEORSHIP_DRAWINGSYSTEM_H

#include <SFML/Graphics/RenderWindow.hpp>

#include "Components/Drawable.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "ECS/World.h"

namespace ms::systems {
    class DrawingSystem : public ecs::System {
    public:
        void update(float deltaTime) override {
            auto& renderWindow = getWorld().getMainWindow();
            getWorld().getECS().view<components::Transform, components::Drawable>().each(
                [&renderWindow](ecs::Entity const& entity, components::Transform& transform, components::Drawable& drawable) {
                    sf::RenderStates states = sf::RenderStates::Default;
                    states.transform *= transform.getTransform();

                    if (drawable.vertices.has_value()) renderWindow.draw(drawable.vertices.value(), states);
                    if (drawable.sprite.has_value()) {
                        auto& animated_sprite = drawable.sprite.value();
                        renderWindow.draw(animated_sprite.sprite, states);
                        animated_sprite.update();
                    }
            });
        }
    };
}
#endif //METEORSHIP_DRAWINGSYSTEM_H
