//
// Created by iformat on 10/9/26.
//

#ifndef METEORSHIP_ANIMATEDSPRITE_H
#define METEORSHIP_ANIMATEDSPRITE_H

#include <iostream>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

namespace ms::utility {
    struct AnimatedSprite {
        sf::Sprite sprite;
        sf::IntRect rect;
        int number_of_frames;
        sf::Clock clock;
        std::optional<std::vector<int>> frame_order;

        //TODO:
        //one sprite sheet
        //idle animation + movement animation

        AnimatedSprite(sf::Texture const& texture, sf::IntRect const _rect, int const _number_of_frames)
            : sprite(texture),
              rect(_rect),
              number_of_frames(_number_of_frames)
        {
            sprite.setTextureRect(rect);
            sprite.setOrigin(sprite.getLocalBounds().getCenter());
        }

        //TODO:
        //i need to change how i move the rect, it goes 1 -> 2 -> 3 -> 1...,
        //and what if i need a custom way to go by frames like
        //1 -> 2 -> 3 -> 2 -> -> 1 or 1 -> 3 -> 1 -> 2 -> 1 ...
        void update() {
            if (clock.getElapsedTime().asSeconds() > 1.0f) {
                if (rect.position.x == rect.size.x * (number_of_frames - 1)) {
                    rect.position.x = 0;
                } else {
                    rect.position.x += rect.size.x;
                }

                sprite.setTextureRect(rect);
                clock.restart();
            }
        }
    };
}

#endif //METEORSHIP_ANIMATEDSPRITE_H
