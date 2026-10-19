//
// Created by iformat on 10/19/26.
//

#ifndef METEORSHIP_UI_H
#define METEORSHIP_UI_H
#include "UINode.h"

struct nk_context;
namespace ms::ui {
    class UI {
    public:
        std::unique_ptr<UINode> root;

        void draw(nk_context* ctx); ;
    };
}

#endif //METEORSHIP_UI_H
