//
// Created by iformat on 10/25/26.
//

#include "SceneMainMenu.h"
#include "Game.h"

namespace ms::scene_logic::scenes {
    SceneMainMenu::SceneMainMenu(Game &game, const std::string &name): SceneBase(game, name) {
        const auto buttonStyle = ui::nineSliceButtonStyle(game.getAssetManager().textureMap.at("red_button"));
        ui.root = std::make_unique<ui::UINode>();
        auto& title = ui.root->add(ui::widgets::LabelWidget{
            .text = "METEORSHIP",
            .align = NK_TEXT_CENTERED
        }, {400.f, 40.f}).alignToParent(ui::Alignment::Center, {0.f, -142.f});
        auto& subtitle = ui.root->add(ui::widgets::LabelWidget{
            .text = "A SPACE SHOOTER",
            .align = NK_TEXT_CENTERED
        }, {400.f, 28.f}).alignTo(title, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {0.f, 5.f});

        auto& playButton = ui.root->add(ui::widgets::ButtonWidget{
            .text = "PLAY",
            .onClick = [this] { this->game.getSceneManager().changeScene("main"); },
            .style = buttonStyle
        }, {180.f, 52.f}).alignTo(subtitle, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {0.f, 24.f});
        auto& settingsButton = ui.root->add(ui::widgets::ButtonWidget{
            .text = "SETTINGS",
            .onClick = [this] { this->game.getSceneManager().changeScene("settings"); },
            .style = buttonStyle
        }, {180.f, 52.f}).alignTo(playButton, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {0.f, 14.f});
        ui.root->add(ui::widgets::ButtonWidget{
            .text = "EXIT",
            .onClick = [this] { this->game.getMainWindow().close(); },// we need to make custom close event or something
            .style = buttonStyle
        }, {180.f, 52.f}).alignTo(settingsButton, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {0.f, 14.f});
    }

    void SceneMainMenu::init() {

    }

    void SceneMainMenu::destroy() {

    }

    void SceneMainMenu::update(float elapsedTime) {
        world->getECS().update(elapsedTime);
    }
}
