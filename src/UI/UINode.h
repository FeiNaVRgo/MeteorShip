#ifndef METEORSHIP_UINODE_H
#define METEORSHIP_UINODE_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "Widget.h"

struct nk_context;

namespace ms::ui {
    enum class Alignment {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    class UINode {
        struct AlignmentConstraint {
            const UINode* target;
            Alignment nodeAlignment;
            Alignment targetAlignment;
            sf::Vector2f offset;
        };

        std::optional<AlignmentConstraint> alignment;

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

        template<typename T>
        UINode& add(T&& widget, const sf::Vector2f nodeSize) {
            auto child = std::make_unique<UINode>(Widget{std::forward<T>(widget)});
            child->size = nodeSize;
            return *addChild(std::move(child));
        }

        template<typename T>
        T* getWidget() {
            return widget ? std::get_if<T>(&*widget) : nullptr;
        }

        template<typename T>
        const T* getWidget() const {
            return widget ? std::get_if<T>(&*widget) : nullptr;
        }

        UINode& alignToParent(Alignment alignment, sf::Vector2f offset = {});
        UINode& alignTo(const UINode& target, Alignment nodeAlignment, Alignment targetAlignment,
                        sf::Vector2f offset = {});

        void render(nk_context* ctx, sf::Vector2f parentAbsPos, sf::Vector2f parentSize);
        void render(nk_context* ctx, sf::Vector2f parentAbsPos);
        void render(nk_context* ctx);
    };
}

#endif //METEORSHIP_UINODE_H