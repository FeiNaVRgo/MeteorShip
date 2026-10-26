//
// Created by iformat on 10/25/26.
//

#include "SceneMainMenu.h"
#include "Game.h"

namespace ms::scene_logic::scenes {
    SceneMainMenu::SceneMainMenu(Game &game, const std::string &name): SceneBase(game, name) {
        game.getAssetManager().textureMap.emplace("red_button", sf::Texture("./assets/UI/red_button.png"));
        struct nk_image texture_img = nk_image_id(static_cast<int>(game.getAssetManager().textureMap.at("red_button").getNativeHandle()));
        nk_ushort border_left   = 12;
        nk_ushort border_top    = 12;
        nk_ushort border_right  = 12;
        nk_ushort border_bottom = 12;
        struct nk_nine_slice button_slice = nk_nine_slice_handle(
                                                texture_img.handle,
                                                border_left,
                                                border_top,
                                                border_right,
                                                border_bottom);

        struct nk_style_button style_button;
        style_button.normal = nk_style_item_nine_slice(button_slice);
        style_button.hover = nk_style_item_nine_slice(button_slice);
        style_button.active = nk_style_item_nine_slice(button_slice);

        ui.root = std::make_unique<ui::UINode>();
        auto btn = std::make_unique<ui::UINode>(ui::widgets::ButtonWidget{.text = "PLAY", .onClick = [this]() {
            this->game.getSceneManager().changeScene("main");
        }, .style = style_button}, sf::Vector2f(50.f, 50.f));
        btn->size = sf::Vector2f(50.f, 50.f);
        ui.root->addChild(std::move(btn));
    }

    void SceneMainMenu::init() {

    }

    void SceneMainMenu::destroy() {

    }

    void SceneMainMenu::update(float elapsedTime) {
        world->getECS().update(elapsedTime);
    }
}
