//
// Created by iformat on 6/19/26.
//

#ifndef METEORSHIP_DRAWABLE_H
#define METEORSHIP_DRAWABLE_H

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "Utility/AnimatedSprite.h"

namespace ms::components {
    struct Drawable {
        std::optional<sf::VertexArray> vertices;
        std::optional<sf::Texture> texture;
        std::optional<utility::AnimatedSprite> sprite;
        //think we should make a field with based on what we are drawing so we know what views to use
        //now we can make static functions for initialization of vertices
    };
}

#endif //METEORSHIP_DRAWABLE_H
