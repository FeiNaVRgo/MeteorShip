//
// Created by iformat on 10/19/26.
//

#include "Widget.h"

#include <algorithm>
#include <SFML/Graphics/Texture.hpp>

void ms::ui::WidgetDrawer::handleClick(
    const bool clicked, const std::function<void()>& onClick) const {
    if (!clicked || !onClick) return;
    if (input) {
        input->registerClick(onClick);
    } else {
        onClick();
    }
}

void ms::ui::WidgetDrawer::operator()(widgets::LabelWidget &w) const {
    if (w.visible) nk_label(ctx, w.text.c_str(), w.align);
}

void ms::ui::WidgetDrawer::operator()(widgets::ButtonWidget &w) const {
    if (w.style.has_value()) {
        nk_style_push_style_item(ctx, &ctx->style.button.normal, w.style.value().normal);
        nk_style_push_style_item(ctx, &ctx->style.button.hover, w.style.value().hover);
        nk_style_push_style_item(ctx, &ctx->style.button.active, w.style.value().active);
        ctx->style.button.border = 0;
    }
    handleClick(nk_button_label(ctx, w.text.c_str()), w.onClick);
    if (w.style.has_value()) {
        nk_style_pop_style_item(ctx);
        nk_style_pop_style_item(ctx);
        nk_style_pop_style_item(ctx);
    }
}

void ms::ui::WidgetDrawer::operator()(widgets::InventorySlotWidget &w) const {
    if (!w.visible) return;
    const float previousBorder = ctx->style.button.border;
    nk_style_push_color(ctx, &ctx->style.button.border_color,
                        w.selected ? nk_rgb(255, 215, 0) : nk_rgb(90, 90, 90));
    ctx->style.button.border = 2.0f;

    bool clicked = false;
    if (w.icon) {
        const auto& texture = w.icon->getTexture();
        const auto textureSize = texture.getSize();
        const auto textureRect = w.icon->getTextureRect();
        const auto image = nk_subimage_handle(
            nk_handle_id(static_cast<int>(texture.getNativeHandle())),
            static_cast<nk_ushort>(textureSize.x),
            static_cast<nk_ushort>(textureSize.y),
            nk_rect(
                static_cast<float>(textureRect.position.x),
                static_cast<float>(textureRect.position.y),
                static_cast<float>(textureRect.size.x),
                static_cast<float>(textureRect.size.y)
            )
        );
        clicked = nk_button_image(ctx, image);
    } else {
        clicked = nk_button_label(ctx, w.itemName.empty() ? "+" : w.itemName.c_str());
    }

    ctx->style.button.border = previousBorder;
    nk_style_pop_color(ctx);

    handleClick(clicked, w.onClick);
}

void ms::ui::WidgetDrawer::operator()(widgets::SliderWidget &w) const { nk_slider_float(ctx, w.min, &w.value, w.max, w.step); }

void ms::ui::WidgetDrawer::operator()(widgets::CheckboxWidget &w) const { nk_checkbox_label(ctx, w.text.c_str(), &w.checked); }

void ms::ui::WidgetDrawer::operator()(widgets::PropertyWidget &w) const { nk_property_int(ctx, w.name.c_str(), w.min, &w.value, w.max, w.step, 1.0f); }

void ms::ui::WidgetDrawer::operator()(widgets::DropdownWidget &w) const {
    if (w.options.empty()) return;

    std::vector<const char*> options;
    options.reserve(w.options.size());
    for (const auto& option : w.options) {
        options.push_back(option.c_str());
    }

    const int previousSelection = w.selected;
    w.selected = std::clamp(
        nk_combo(ctx, options.data(), static_cast<int>(options.size()), w.selected,
                 w.itemHeight, nk_vec2(360.f, 240.f)),
        0, static_cast<int>(options.size()) - 1);
    if (w.selected != previousSelection && w.onSelectionChanged) {
        w.onSelectionChanged(w.selected);
    }
}