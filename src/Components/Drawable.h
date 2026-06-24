//
// Created by iformat on 6/19/26.
//

#ifndef METEORSHIP_DRAWABLE_H
#define METEORSHIP_DRAWABLE_H

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace ms::components {
    struct Drawable {
        sf::VertexArray vertices;
        sf::Texture texture;
        //I think we should make a field with based on what we are drawing so we know what views to use
        //now we can make static functions for initialization of vertices
    };
}

#endif //METEORSHIP_DRAWABLE_H
