//
// Created by iformat on 10/25/26.
//

#include "SceneMainMenu.h"
#include "Game.h"

namespace ms::scene_logic::scenes {
    SceneMainMenu::SceneMainMenu(Game &game, const std::string &name): SceneBase(game, name) {
        ui.root = std::make_unique<ui::UINode>();
        auto btn = std::make_unique<ui::UINode>(ui::widgets::ButtonWidget{.text = "PLAY", .onClick = [this]() {
            this->game.getSceneManager().changeScene("main");
        }}, sf::Vector2f(50.f, 50.f));
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
