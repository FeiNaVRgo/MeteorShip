//
// Created by iformat on 6/2/26.
//

#include "Game.h"

#include "nuklear_config.h"

#define NK_SFML_GL3_IMPLEMENTATION
#include "nuklear_sfml_gl3.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#include <glad/glad.h>
#include <behaviortree_cpp/bt_factory.h>
#include <pybind11/embed.h>

#include "Components/Drawable.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/Player.h"
#include "Components/ShootingAccess.h"
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"

namespace py = pybind11;

namespace ms
{
    Game::Game(): worldBox(),
                  settings(24, 8, 4, 3, 3),
                  mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default, sf::State::Windowed, settings),
                  eventHandler(*this),
                  sceneManager(worldBox.getWorlds()) {
        mainWindow.setFramerateLimit(60);
        mainWindow.setVerticalSyncEnabled(true);
        auto b = mainWindow.setActive(true);

        //necessary for nuklear ui to work
        if(!gladLoadGL()) {
            printf("Failed to load OpenGL extensions!\n");
            return;
        }
        glViewport(0, 0, mainWindow.getSize().x, mainWindow.getSize().y);
        ctx = nk_sfml_init(&mainWindow);
        nk_sfml_font_stash_begin(&atlas);
        nk_sfml_font_stash_end();


        assetManager.loadAssets();

        //scenes need to be added before registration of components
        sceneManager.addScene(std::make_unique<scene_logic::scenes::SceneMain>(*this, "main"));

        worldBox.registerComponent<components::Rigidbody>();
        worldBox.registerComponent<components::Transform>();
        worldBox.registerComponent<components::Drawable>();
        worldBox.registerComponent<components::Player>();
        worldBox.registerComponent<components::ShootingAccess>();

        sceneManager.changeScene("main");

        BT::BehaviorTreeFactory factory;
    }

    sf::RenderWindow& Game::getMainWindow() {
        return mainWindow;
    }

    void Game::update(sf::Time const &elapsed) const {
        sceneManager.updateCurrentScene(elapsed.asSeconds());
    }

    void Game::handleEvents() {
        nk_input_begin(ctx);
        while (auto event = mainWindow.pollEvent()) {
            eventHandler.handleEvent(event);
            if (event.has_value()) nk_sfml_handle_event(&event.value());
        }
        nk_input_end(ctx);

        while (auto const event = sceneManager.getCurrentScene()->getWorld()->getEventDispatcher().poll()) {
            eventHandler.handleEvent(event);
        }
    }

    void Game::run() {
        sf::Clock clock;

        nk_colorf bg{};
        bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

        py::scoped_interpreter guard{};
        py::object scope = py::module::import("__main__").attr("__dict__");
        py::eval_file("./scripts/example.py", scope);

        while (mainWindow.isOpen()) {
            sf::Time elapsed = clock.restart();

            handleEvents();

            //TODO: make c++ bindings to nuklear ui
            //make ui class that can store ui context
            sceneManager.getCurrentScene()->drawUI(ctx);

            auto isActiveWindow = mainWindow.setActive(true);
            mainWindow.clear(sf::Color(bg.r * 255, bg.g * 255, bg.b * 255, bg.a * 255));

            update(elapsed);

            mainWindow.popGLStates();
            nk_sfml_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
            mainWindow.pushGLStates();


            mainWindow.display();
        }
        nk_sfml_shutdown();
    }
} // ms