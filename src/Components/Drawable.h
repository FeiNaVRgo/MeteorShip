//
// Created by iformat on 6/19/26.
//

#ifndef METEORSHIP_DRAWABLE_H
#define METEORSHIP_DRAWABLE_H

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace ms::component {
    struct Drawable {
        sf::VertexArray vertices;
        sf::Texture texture;
    };
}

#endif //METEORSHIP_DRAWABLE_H
