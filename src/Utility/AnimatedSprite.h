//
// Created by iformat on 10/9/26.
//

#ifndef METEORSHIP_ANIMATEDSPRITE_H
#define METEORSHIP_ANIMATEDSPRITE_H

#include <iostream>
#include <optional>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

namespace ms::utility {
    struct AnimatedSprite {
        sf::Sprite sprite;
        sf::IntRect rect;
        int number_of_sprites;
        sf::Clock clock;
        std::optional<std::vector<int>> order_of_sprites;
        int i = 0;

        //TODO:
        //one sprite sheet
        //idle animation + movement animation

        AnimatedSprite(sf::Texture const& texture,
                       sf::IntRect const _rect,
                       int const _number_of_sprites,
                       std::optional<std::vector<int>> const& _order_of_sprites)
            : sprite(texture),
              rect(_rect),
              number_of_sprites(_number_of_sprites),
              order_of_sprites(_order_of_sprites)
        {
            if (order_of_sprites && !order_of_sprites->empty()) {
                rect.position.x = rect.size.x * order_of_sprites->front();
            }
            sprite.setTextureRect(rect);
            sprite.setOrigin(sprite.getLocalBounds().getCenter());
        }

        AnimatedSprite createInstance() const {
            return AnimatedSprite(sprite.getTexture(), rect, number_of_sprites, order_of_sprites);
        }

        void update() {
            if (number_of_sprites <= 1 ||
                (order_of_sprites && order_of_sprites->empty())) {
                return;
            }
            if (clock.getElapsedTime().asSeconds() > 1.0f) {
                if (!order_of_sprites.has_value()) {
                    if (rect.position.x == rect.size.x * ( number_of_sprites - 1)) {
                        rect.position.x = 0;
                    } else {
                        rect.position.x += rect.size.x;
                    }
                } else {
                    auto const& vec = order_of_sprites.value();
                    rect.position.x = rect.size.x * vec[i];
                    if (i == vec.size() - 1) {
                        i = 0;
                    } else {
                        i++;
                    }
                }

                sprite.setTextureRect(rect);
                clock.restart();
            }
        }
    };
}

#endif //METEORSHIP_ANIMATEDSPRITE_H
