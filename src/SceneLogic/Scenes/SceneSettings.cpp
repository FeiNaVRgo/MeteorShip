//
// Created by iformat on 10/26/26.
//

#include "SceneSettings.h"

#include "Game.h"

namespace ms::scene_logic::scenes {
    SceneSettings::SceneSettings(Game &game, const std::string &name)
        : SceneBase(game, name) {
        const auto buttonStyle = ui::nineSliceButtonStyle(game.getAssetManager().textureMap.at("red_button"));
        ui.root = std::make_unique<ui::UINode>();
        auto& title = ui.root->add(ui::widgets::LabelWidget{
            .text = "SETTINGS",
            .align = NK_TEXT_CENTERED
        }, {400.f, 40.f})
            .alignToParent(ui::Alignment::Center, {0.f, -85.f});
        auto& placeholder = ui.root->add(ui::widgets::LabelWidget{
            .text = "Settings are not available yet.",
            .align = NK_TEXT_CENTERED
        }, {400.f, 30.f})
            .alignTo(title, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {0.f, 8.f});
        ui.root->add(ui::widgets::ButtonWidget{
            .text = "BACK",
            .onClick = [this] { this->game.getSceneManager().changeScene("main_menu"); },
            .style = buttonStyle
        }, {180.f, 52.f}).alignTo(placeholder, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {0.f, 34.f});
    }

    void SceneSettings::init() {}

    void SceneSettings::destroy() {}

    void SceneSettings::update(float elapsedTime) {
        world->getECS().update(elapsedTime);
    }
}
