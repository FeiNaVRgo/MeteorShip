//
// Created by iformat on 10/19/26.
//

#include "UI.h"

#include <climits>

#include "nuklear_config.h"

namespace ms::ui {
    nk_style_button nineSliceButtonStyle(const sf::Texture& texture, const nk_ushort border) {
        const auto textureSize = texture.getSize();
        const auto slice = nk_sub9slice_handle(
            nk_handle_id(static_cast<int>(texture.getNativeHandle())),
            static_cast<nk_ushort>(textureSize.x),
            static_cast<nk_ushort>(textureSize.y),
            nk_rect(0.0f, 0.0f, static_cast<float>(textureSize.x), static_cast<float>(textureSize.y)),
            border, border, border, border
        );

        nk_style_button style{};
        style.normal = nk_style_item_nine_slice(slice);
        style.hover = nk_style_item_nine_slice(slice);
        style.active = nk_style_item_nine_slice(slice);
        return style;
    }

    void UI::draw(nk_context *ctx, const sf::Vector2u windowSize) {
        nk_style *s = &ctx->style;
        nk_style_push_color(ctx, &s->window.background, nk_rgba(0,0,0,0));
        nk_style_push_style_item(ctx, &s->window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,0)));
        const struct nk_rect windowBounds{
            0.0f,
            0.0f,
            static_cast<float>(windowSize.x),
            static_cast<float>(windowSize.y)
        };
        nk_window_set_bounds(ctx, "Demo", windowBounds);
        if (nk_begin(ctx, "Demo", windowBounds, NK_WINDOW_NO_SCROLLBAR))
        {
            struct nk_rect content = nk_window_get_content_region(ctx);
            nk_layout_space_begin(ctx, NK_STATIC, content.h, INT_MAX);
            UIInputState input;
            if (root) root->render(ctx, {0.0f, 0.0f}, {windowBounds.w, windowBounds.h}, &input);
            nk_layout_space_end(ctx);
            input.dispatchClick();
        }
        nk_end(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
    }
}
