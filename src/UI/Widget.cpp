//
// Created by iformat on 10/19/26.
//

#include "Widget.h"


void ms::ui::WidgetDrawer::operator()(widgets::LabelWidget &w) const { nk_label(ctx, w.text.c_str(), w.align); }

void ms::ui::WidgetDrawer::operator()(widgets::ButtonWidget &w) const { if (nk_button_label(ctx, w.text.c_str()) && w.onClick) w.onClick(); }

void ms::ui::WidgetDrawer::operator()(widgets::SliderWidget &w) const { nk_slider_float(ctx, w.min, &w.value, w.max, w.step); }

void ms::ui::WidgetDrawer::operator()(widgets::CheckboxWidget &w) const { nk_checkbox_label(ctx, w.text.c_str(), &w.checked); }

void ms::ui::WidgetDrawer::operator()(widgets::PropertyWidget &w) const { nk_property_int(ctx, w.name.c_str(), w.min, &w.value, w.max, w.step, 1.0f); }