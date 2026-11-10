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
                                    }, {400.f, 40.f});
        title.alignToParent(ui::Alignment::Center, {0.f, -160.f});

        auto gpuOptions = game.getCollisionDetector().getAvailableDeviceNames();
        if (gpuOptions.empty()) {
            gpuOptions.push_back("CPU fallback");
        }

        auto& gpuLabel = ui.root->add(ui::widgets::LabelWidget{
                                          .text = "GPU",
                                          .align = NK_TEXT_LEFT
                                      }, {120.f, 28.f});
        gpuLabel.alignTo(title, ui::Alignment::TopCenter, ui::Alignment::BottomCenter, {-110.f, 28.f});

        auto& gpuDropdown = ui.root->add(ui::widgets::DropdownWidget{
                                            .options = gpuOptions,
                                            .selected = 0,
                                            .onSelectionChanged = [this](const int selected) {
                                                const auto devices = this->game.getCollisionDetector().getAvailableDeviceNames();
                                                if (devices.empty()) {
                                                    return;
                                                }
                                                this->game.getCollisionDetector().selectDevice(static_cast<std::size_t>(selected));
                                            },
                                            .itemHeight = 24
                                        }, {240.f, 28.f});
        gpuDropdown.alignTo(gpuLabel, ui::Alignment::CenterLeft, ui::Alignment::CenterRight, {8.f, 0.f});

        ui.root->add(ui::widgets::ButtonWidget{
                         .text = "BACK",
                         .onClick = [this] { this->game.getSceneManager().changeScene("main_menu"); },
                         .style = buttonStyle
                     }, {180.f, 52.f}).alignToParent(ui::Alignment::Center, {0.f, 80.f});
    }

    void SceneSettings::init() {
    }

    void SceneSettings::destroy() {
    }

    void SceneSettings::update(float elapsedTime) {
        world->getECS().update(elapsedTime);
    }
}
