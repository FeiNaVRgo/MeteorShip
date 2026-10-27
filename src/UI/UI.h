//
// Created by iformat on 10/19/26.
//

#ifndef METEORSHIP_UI_H
#define METEORSHIP_UI_H
#include "UINode.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

struct nk_context;
namespace ms::ui {
    nk_style_button nineSliceButtonStyle(const sf::Texture& texture, nk_ushort border = 5);

    class UI {
    public:
        std::unique_ptr<UINode> root;

        void draw(nk_context* ctx, sf::Vector2u windowSize);
    };
}

#endif //METEORSHIP_UI_H
