#include "UINode.h"

#include <algorithm>

#include "nuklear_config.h"

namespace ms::ui {
    static sf::Vector2f alignmentFactor(const Alignment alignment) {
        switch (alignment) {
            case Alignment::TopLeft:      return {0.0f, 0.0f};
            case Alignment::TopCenter:    return {0.5f, 0.0f};
            case Alignment::TopRight:     return {1.0f, 0.0f};
            case Alignment::CenterLeft:   return {0.0f, 0.5f};
            case Alignment::Center:       return {0.5f, 0.5f};
            case Alignment::CenterRight:  return {1.0f, 0.5f};
            case Alignment::BottomLeft:   return {0.0f, 1.0f};
            case Alignment::BottomCenter: return {0.5f, 1.0f};
            case Alignment::BottomRight:  return {1.0f, 1.0f};
        }
        return {};
    }

    UINode::UINode() = default;
    UINode::UINode(Widget w, sf::Vector2f pos)
        : relPos(pos), widget(std::move(w)) {}

    UINode* UINode::addChild(std::unique_ptr<UINode> child) {
        children.push_back(std::move(child));
        return children.back().get();
    }

    UINode& UINode::alignToParent(const Alignment alignment, const sf::Vector2f offset) {
        this->alignment = AlignmentConstraint{.target = nullptr, .nodeAlignment = alignment, .targetAlignment = alignment, .offset = offset};
        return *this;
    }

    UINode& UINode::alignTo(const UINode& target, const Alignment nodeAlignment,
                            const Alignment targetAlignment, const sf::Vector2f offset) {
        alignment = AlignmentConstraint{.target = &target, .nodeAlignment = nodeAlignment, .targetAlignment = targetAlignment, .offset = offset};
        return *this;
    }

    static void drawWidgetAt(nk_context* ctx, const struct nk_rect& rect, Widget& w) {
        nk_layout_space_push(ctx, rect);
        std::visit(WidgetDrawer{ctx}, w);
    }

    void UINode::render(nk_context* ctx, const sf::Vector2f parentAbsPos, const sf::Vector2f parentSize) {
        float w = size.x;
        float h = size.y;

        if (w <= 0.0f) w = std::max(0.0f, parentSize.x - relPos.x);
        if (h <= 0.0f) h = widget ? 25.0f : std::max(0.0f, parentSize.y - relPos.y);

        sf::Vector2f position = parentAbsPos + relPos;
        if (alignment) {
            const sf::Vector2f targetPos = alignment->target
                ? alignment->target->absPos
                : parentAbsPos;
            const sf::Vector2f targetSize = alignment->target
                ? alignment->target->absSize
                : parentSize;
            const sf::Vector2f ownFactor = alignmentFactor(alignment->nodeAlignment);
            const sf::Vector2f targetFactor = alignmentFactor(alignment->targetAlignment);
            position = targetPos + sf::Vector2f(targetSize.x * targetFactor.x, targetSize.y * targetFactor.y)
                - sf::Vector2f(w * ownFactor.x, h * ownFactor.y)
                + alignment->offset;
        }
        absPos = position;

        struct nk_rect rect{
            .x = position.x,
            .y = position.y,
            .w = w,
            .h = h
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
            child->render(ctx, absPos, absSize);
        }
    }

    void UINode::render(nk_context* ctx, const sf::Vector2f parentAbsPos) {
        const struct nk_rect content = nk_window_get_content_region(ctx);
        render(ctx, parentAbsPos, {content.w, content.h});
    }

    void UINode::render(nk_context* ctx) {
        const struct nk_rect content = nk_window_get_content_region(ctx);
        render(ctx, {0.0f, 0.0f}, {content.w, content.h});
    }
}