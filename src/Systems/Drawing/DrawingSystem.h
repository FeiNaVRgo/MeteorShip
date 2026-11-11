//
// Created by iformat on 6/24/26.
//

#ifndef METEORSHIP_DRAWINGSYSTEM_H
#define METEORSHIP_DRAWINGSYSTEM_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Drawable.h"
#include "Components/Hitbox.h"
#include "Components/Transform.h"
#include "ECS/ECS.h"
#include "ECS/World.h"

namespace ms::systems {
    class DrawingSystem : public ecs::System {
    public:
        static void drawBoundingBox(
            sf::RenderTarget& target,
            const components::BoundingBox& boundingBox) {
            sf::RectangleShape rectangle(boundingBox.bounds.size);
            rectangle.setPosition(boundingBox.bounds.position);
            rectangle.setFillColor(sf::Color::Transparent);
            rectangle.setOutlineColor(sf::Color::Green);
            rectangle.setOutlineThickness(1.f);
            target.draw(rectangle);
        }

        static void drawCollisionShape(
            sf::RenderTarget& target,
            const components::CollisionShape& collisionShape,
            const components::Transform& transform) {
            if (collisionShape.getVertices().size() < 3) {
                return;
            }

            sf::VertexArray outline(
                sf::PrimitiveType::LineStrip, collisionShape.getVertices().size() + 1);
            const auto& transformMatrix = transform.getTransform();
            for (std::size_t i = 0; i <= collisionShape.getVertices().size(); ++i) {
                outline[i].position = transformMatrix.transformPoint(
                    collisionShape.getVertices()[i % collisionShape.getVertices().size()]);
                outline[i].color = sf::Color::Yellow;
            }
            target.draw(outline);
        }

        static void drawHitboxes(
            sf::RenderTarget& target,
            const components::Hitbox& hitbox,
            const components::Transform& transform) {
            sf::RenderStates states = sf::RenderStates::Default;
            states.transform *= transform.getTransform();

            for (const auto& bounds : hitbox.rectangles) {
                const auto horizontal = bounds.axes[0] * bounds.extents.x;
                const auto vertical = bounds.axes[1] * bounds.extents.y;
                const sf::Vector2f corners[] = {
                    bounds.center - horizontal - vertical,
                    bounds.center + horizontal - vertical,
                    bounds.center + horizontal + vertical,
                    bounds.center - horizontal + vertical,
                    bounds.center - horizontal - vertical
                };
                sf::VertexArray outline(sf::PrimitiveType::LineStrip, 5);
                for (std::size_t i = 0; i < 5; ++i) {
                    outline[i].position = corners[i];
                    outline[i].color = sf::Color::Magenta;
                }
                target.draw(outline, states);
            }
        }

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

            auto& entityManager = getWorld().getECS();
            entityManager.view<components::BoundingBox>().each(
                [&renderWindow](
                    ecs::Entity const&,
                    const components::BoundingBox& boundingBox) {
                    drawBoundingBox(renderWindow, boundingBox);
                });
            entityManager.view<components::CollisionShape, components::Transform>().each(
                [&renderWindow](
                    ecs::Entity const&,
                    const components::CollisionShape& collisionShape,
                    const components::Transform& transform) {
                    drawCollisionShape(renderWindow, collisionShape, transform);
                });
            entityManager.view<components::Hitbox, components::Transform>().each(
                [&renderWindow](
                    ecs::Entity const&,
                    const components::Hitbox& hitbox,
                    const components::Transform& transform) {
                    drawHitboxes(renderWindow, hitbox, transform);
                });
        }
    };
}
#endif //METEORSHIP_DRAWINGSYSTEM_H
