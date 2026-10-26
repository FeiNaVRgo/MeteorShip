//
// Created by iformat on 10/19/26.
//

#include "Widget.h"


void ms::ui::WidgetDrawer::operator()(widgets::LabelWidget &w) const { nk_label(ctx, w.text.c_str(), w.align); }

void ms::ui::WidgetDrawer::operator()(widgets::ButtonWidget &w) const {
    if (w.style.has_value()) {
        nk_style_push_style_item(ctx, &ctx->style.button.normal, w.style.value().normal);
        nk_style_push_style_item(ctx, &ctx->style.button.hover, w.style.value().hover);
        nk_style_push_style_item(ctx, &ctx->style.button.active, w.style.value().active);
        ctx->style.button.border = 0;
    }
    if (nk_button_label(ctx, w.text.c_str()) && w.onClick) {
        w.onClick();
    }
    if (w.style.has_value()) {
        nk_style_pop_style_item(ctx);
        nk_style_pop_style_item(ctx);
        nk_style_pop_style_item(ctx);
    }
}

void ms::ui::WidgetDrawer::operator()(widgets::SliderWidget &w) const { nk_slider_float(ctx, w.min, &w.value, w.max, w.step); }

void ms::ui::WidgetDrawer::operator()(widgets::CheckboxWidget &w) const { nk_checkbox_label(ctx, w.text.c_str(), &w.checked); }

void ms::ui::WidgetDrawer::operator()(widgets::PropertyWidget &w) const { nk_property_int(ctx, w.name.c_str(), w.min, &w.value, w.max, w.step, 1.0f); }