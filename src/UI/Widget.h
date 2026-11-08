//
// Created by iformat on 10/19/26.
//

#ifndef METEORSHIP_WIDGET_H
#define METEORSHIP_WIDGET_H
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include "nuklear_config.h"

namespace ms::ui {
    namespace widgets {
        struct LabelWidget {
            std::string text;
            nk_flags align = NK_TEXT_LEFT;
            bool visible = true;
        };

        struct ButtonWidget {
            std::string text;
            std::function<void()> onClick;
            std::optional<nk_style_button> style;
        };

        struct InventorySlotWidget {
            std::string itemName;
            std::optional<sf::Sprite> icon;
            std::function<void()> onClick;
            bool selected = false;
            bool visible = true;
        };

        struct TurretStatusWidget {
            float mana = 0.f;
            float manaMax = 0.f;
            float rechargeRemaining = 0.f;
            float rechargeTime = 0.f;
            bool visible = false;
        };

        struct SliderWidget {
            float value = 0.5f;
            float min = 0.0f;
            float max = 1.0f;
            float step = 0.01f;
        };

        struct CheckboxWidget {
            std::string text;
            int checked = false;
        };

        struct PropertyWidget {
            std::string name;
            int min = 0, max = 100, value = 0, step = 1;
        };

        struct DropdownWidget {
            std::vector<std::string> options;
            int selected{};
            std::function<void(int)> onSelectionChanged;
            int itemHeight = 24;
        };
    }

    using Widget = std::variant<
        widgets::LabelWidget,
        widgets::ButtonWidget,
        widgets::InventorySlotWidget,
        widgets::TurretStatusWidget,
        widgets::SliderWidget,
        widgets::CheckboxWidget,
        widgets::PropertyWidget,
        widgets::DropdownWidget
    >;

    class UIInputState {
        std::function<void()> topmostClick;

    public:
        void registerClick(const std::function<void()>& onClick) {
            topmostClick = onClick;
        }

        void dispatchClick() {
            if (!topmostClick) return;
            auto onClick = std::move(topmostClick);
            topmostClick = {};
            onClick();
        }
    };

    struct WidgetDrawer {
        nk_context* ctx;
        UIInputState* input = nullptr;

        void handleClick(bool clicked, const std::function<void()>& onClick) const;
        void operator()(widgets::LabelWidget& w)    const;

        void operator()(widgets::ButtonWidget& w)   const;

        void operator()(widgets::InventorySlotWidget& w) const;

        void operator()(widgets::TurretStatusWidget& w) const;

        void operator()(widgets::SliderWidget& w)   const;

        void operator()(widgets::CheckboxWidget& w) const;

        void operator()(widgets::PropertyWidget& w) const;

        void operator()(widgets::DropdownWidget& w) const;
    };
}

#endif //METEORSHIP_WIDGET_H
