#ifndef METEORSHIP_UINODE_H
#define METEORSHIP_UINODE_H

#include <memory>
#include <optional>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "Widget.h"

struct nk_context;

namespace ms::ui {

    class UINode {
    public:
        sf::Vector2f relPos{0.0f, 0.0f};
        sf::Vector2f size{0.0f, 0.0f};

        sf::Vector2f absPos{0.0f, 0.0f};
        sf::Vector2f absSize{0.0f, 0.0f};

        std::vector<std::unique_ptr<UINode>> children;
        std::optional<Widget> widget;

        UINode();
        explicit UINode(Widget w, sf::Vector2f pos = {0.0f, 0.0f});

        //TODO
        //adding nodes by name, basically i bind a name to a Node
        //and when i am adding another node i can add it to an existing node by name
        UINode* addChild(std::unique_ptr<UINode> child);

        //TODO
        //rn when rendering in position {0, 0} there is still some height applied, fix it
        void render(nk_context* ctx, sf::Vector2f parentAbsPos);
        void render(nk_context* ctx);
    };
}

#endif //METEORSHIP_UINODE_H