//
// Created by iformat on 10/19/26.
//

#ifndef METEORSHIP_WIDGET_H
#define METEORSHIP_WIDGET_H
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include "nuklear_config.h"

namespace ms::ui {
    namespace widgets {
        struct LabelWidget {
            std::string text;
            nk_flags align = NK_TEXT_LEFT;
        };

        struct ButtonWidget {
            std::string text;
            std::function<void()> onClick;
            std::optional<nk_style_button> style;
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
    }

    using Widget = std::variant<
        widgets::LabelWidget,
        widgets::ButtonWidget,
        widgets::SliderWidget,
        widgets::CheckboxWidget,
        widgets::PropertyWidget
    >;

    struct WidgetDrawer {
        nk_context* ctx;
        void operator()(widgets::LabelWidget& w)    const;

        void operator()(widgets::ButtonWidget& w)   const;

        void operator()(widgets::SliderWidget& w)   const;

        void operator()(widgets::CheckboxWidget& w) const;

        void operator()(widgets::PropertyWidget& w) const;
    };
}

#endif //METEORSHIP_WIDGET_H
