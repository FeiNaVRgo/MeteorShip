//
// Created by iformat on 6/2/26.
//

#include "Game.h"

#include "Components/Projectile.h"
#include "Components/TurretMountLayout.h"
#include "nuklear_config.h"

#define NK_SFML_GL3_IMPLEMENTATION
#include "nuklear_sfml_gl3.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#include <glad/glad.h>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/xml_parsing.h>
#include <pybind11/embed.h>
#include <string>

#include "BTNodes/IsPlayerVisible.h"
#include "BTNodes/MoveForward.h"
#include "BTNodes/MoveTowards.h"
#include "BTNodes/RandomMovement.h"
#include "BTNodes/RotateTowards.h"
#include "Components/BehaviorTree.h"
#include "Components/Drawable.h"
#include "Components/Enemy.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/Player.h"
#include "Events/EventDispatcher.h"
#include "SceneLogic/Scenes/SceneMainMenu.h"
#include "SceneLogic/Scenes/SceneSettings.h"
#include "Systems/SpatialHashing/Quadtree.h"

namespace py = pybind11;

namespace ms
{
    Game::Game(): worldBox(),
                  settings(24, 8, 4, 3, 3),
                  mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default, sf::State::Windowed, settings),
                  eventHandler(*this),
                  sceneManager(worldBox.getWorlds()),
                  globalBlackboard(BT::Blackboard::create()) {

        {
            //here is where we register all of our nodes for behavior
            factory.registerNodeType<bt_nodes::IsPlayerVisible>("IsPlayerVisible");
            factory.registerNodeType<bt_nodes::RotateTowards>("RotateTowards");
            factory.registerNodeType<bt_nodes::MoveForward>("MoveForward");
            factory.registerNodeType<bt_nodes::MoveTowards>("MoveTowards");
            factory.registerNodeType<bt_nodes::RandomMovement>("RandomMovement");
            std::string xml_models = BT::writeTreeNodesModelXML(factory);
            std::ofstream out("models.xml");
            if (out.is_open()) {
                out << xml_models;
                out.close();
            }
        }
        globalBlackboard->set("game_ptr", this);

        //mainWindow.setFramerateLimit(60);
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
        sceneManager.addScene<scene_logic::scenes::SceneMain>(*this, "main");
        sceneManager.addScene<scene_logic::scenes::SceneMainMenu>(*this, "main_menu");
        sceneManager.addScene<scene_logic::scenes::SceneSettings>(*this, "settings");

        worldBox.registerComponent<components::Rigidbody>();
        worldBox.registerComponent<components::Transform>();
        worldBox.registerComponent<components::Drawable>();
        worldBox.registerComponent<components::Player>();
        worldBox.registerComponent<components::BehaviorTree>();
        worldBox.registerComponent<components::Enemy>();
        worldBox.registerComponent<components::TurretMountLayout>();
        worldBox.registerComponent<components::Projectile>();

        sceneManager.changeScene("main_menu");
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
        sf::Time fpsSampleTime = sf::Time::Zero;
        unsigned int fpsSampleFrames = 0;
        unsigned int fps = 0;
        bool hasFpsSample = false;

        nk_colorf bg{};
        bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

        /*
        py::scoped_interpreter guard{};
        auto cppModule = py::module_::import("CPPModule");

        std::ifstream file("./scripts/example.py");
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string scriptSource = buffer.str();

        py::dict globals = cppModule.attr("__dict__");

        py::exec(scriptSource, globals);
        py::object updateFunc = globals["func"];
        updateFunc();
        */



        while (mainWindow.isOpen()) {
            sf::Time elapsed = clock.restart();
            fpsSampleTime += elapsed;
            ++fpsSampleFrames;
            if (fpsSampleTime >= sf::seconds(0.5f)) {
                fps = static_cast<unsigned int>(
                    static_cast<float>(fpsSampleFrames) / fpsSampleTime.asSeconds());
                fpsSampleTime = sf::Time::Zero;
                fpsSampleFrames = 0;
                hasFpsSample = true;
            }

            handleEvents();
            if (closeRequested) break;//very not elegant qwp

            sceneManager.getCurrentScene()->drawUI(ctx); // render of ui, puts stuff into nuklear buffer that will be drawn later
            const auto windowSize = mainWindow.getSize();
            const struct nk_rect fpsBounds{
                static_cast<float>(windowSize.x) - 100.0f,
                static_cast<float>(windowSize.y) - 34.0f,
                92.0f,
                26.0f
            };
            nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0, 0, 0, 0));
            nk_style_push_style_item(ctx, &ctx->style.window.fixed_background,
                                     nk_style_item_color(nk_rgba(0, 0, 0, 0)));
            if (nk_begin(ctx, "FPS", fpsBounds, NK_WINDOW_NO_INPUT | NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 22.0f, 1);
                const std::string fpsLabel = hasFpsSample ? "FPS: " + std::to_string(fps) : "FPS: --";
                nk_label(ctx, fpsLabel.c_str(), NK_TEXT_ALIGN_RIGHT | NK_TEXT_ALIGN_MIDDLE);
            }
            nk_end(ctx);
            nk_style_pop_color(ctx);
            nk_style_pop_style_item(ctx);

            auto isActiveWindow = mainWindow.setActive(true);
            mainWindow.clear(sf::Color(bg.r * 255, bg.g * 255, bg.b * 255, bg.a * 255));

            update(elapsed);

            mainWindow.popGLStates();
            nk_sfml_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);//actual render of ui
            mainWindow.pushGLStates();


            mainWindow.display();
        }
        (void)mainWindow.setActive(true);
        nk_sfml_shutdown();
        mainWindow.close();
    }
} // ms