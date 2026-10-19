//
// Created by iformat on 10/19/26.
//

#include "UI.h"

#include <climits>

#include "nuklear_config.h"

namespace ms::ui {
    void UI::draw(nk_context *ctx) {
        nk_style *s = &ctx->style;
        nk_style_push_color(ctx, &s->window.background, nk_rgba(0,0,0,0));
        nk_style_push_style_item(ctx, &s->window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,0)));
        if (nk_begin(ctx, "Demo", nk_rect(0, 0, 800, 600), NK_WINDOW_NO_SCROLLBAR))
        {
            struct nk_rect content = nk_window_get_content_region(ctx);
            nk_layout_space_begin(ctx, NK_STATIC, content.h, INT_MAX);
            if (root) root->render(ctx);
            nk_layout_space_end(ctx);
        }
        nk_end(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
    }
}
