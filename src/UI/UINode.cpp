#include "UINode.h"

#include "nuklear_config.h"

namespace ms::ui {

    UINode::UINode() = default;
    UINode::UINode(Widget w, sf::Vector2f pos)
        : widget(std::move(w)), relPos(pos) {}

    UINode* UINode::addChild(std::unique_ptr<UINode> child) {
        children.push_back(std::move(child));
        return children.back().get();
    }

    static void drawWidgetAt(nk_context* ctx, const struct nk_rect& rect, Widget& w) {
        nk_layout_space_push(ctx, rect);
        std::visit(WidgetDrawer{ctx}, w);
    }

    void UINode::render(nk_context* ctx, sf::Vector2f parentAbsPos) {
        absPos = parentAbsPos + relPos;
        float w = size.x;
        float h = size.y;

        struct nk_rect content = nk_window_get_content_region(ctx);
        if (w <= 0.0f) w = content.w - relPos.x;
        if (h <= 0.0f) h = 25.0f;

        struct nk_rect rect{
            absPos.x,
            absPos.y,
            w,
            h
        };

        if (widget) {
            drawWidgetAt(ctx, rect, *widget);
            if (size.x <= 0.0f || size.y <= 0.0f) {
                struct nk_rect used = nk_widget_bounds(ctx);
                if (size.x <= 0.0f) rect.w = used.w;
                if (size.y <= 0.0f) rect.h = used.h;
            }
        }

        absSize = {rect.w, rect.h};
        
        for (auto& child : children) {
            child->render(ctx, absPos);
        }
    }

    void UINode::render(nk_context* ctx) {
        render(ctx, sf::Vector2f{0.0f, 0.0f});
    }
}