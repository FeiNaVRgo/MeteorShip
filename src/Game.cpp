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
#include <sol/sol.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "BTNodes/IsPlayerVisible.h"
#include "BTNodes/MoveForward.h"
#include "BTNodes/MoveTowards.h"
#include "BTNodes/RandomMovement.h"
#include "BTNodes/RotateTowards.h"
#include "BTNodes/LuaBehaviorNode.h"
#include "Components/BehaviorTree.h"
#include "Components/BoundingBox.h"
#include "Components/CollisionShape.h"
#include "Components/Damage.h"
#include "Components/Drawable.h"
#include "Components/Enemy.h"
#include "Components/Hitbox.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/Player.h"
#include "Components/Scene.h"
#include "Events/EventDispatcher.h"
#include "SceneLogic/Scenes/SceneMainMenu.h"
#include "SceneLogic/Scenes/SceneSettings.h"
#include "../Bindings/LuaBinding/lua_bindings.h"
#include "Utility/SaveState/SavePath.h"

namespace py = pybind11;

namespace ms
{
    Game::Game(): worldBox(),
                  sfml_ctx_settings(24, 8, 4, 3, 3),
                  mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default, sf::State::Windowed, sfml_ctx_settings),
                  eventHandler(*this),
                  sceneManager(worldBox.getWorlds()),
                  globalBlackboard(BT::Blackboard::create()),
                  settings(utility::save_dir("MeteorShip").value_or({}) / "config" / "settings.json") {

        lua.open_libraries(sol::lib::base, sol::lib::package);
        scripting::lua_bind(lua, *this);
        bt_nodes::registerLuaBehaviorNodes(factory, *this);

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

        //mainWindow.setFramerateLimit(10);
        //mainWindow.setVerticalSyncEnabled(true);
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
        worldBox.registerComponent<components::BoundingBox>();
        worldBox.registerComponent<components::CollisionShape>();
        worldBox.registerComponent<components::Hitbox>();
        worldBox.registerComponent<components::Transform>();
        worldBox.registerComponent<components::Drawable>();
        worldBox.registerComponent<components::Player>();
        worldBox.registerComponent<components::BehaviorTree>();
        worldBox.registerComponent<components::Enemy>();
        worldBox.registerComponent<components::TurretMountLayout>();
        worldBox.registerComponent<components::Projectile>();
        worldBox.registerComponent<components::DamageAmount>();
        worldBox.registerComponent<components::Scene>();

        sceneManager.changeScene("main_menu");

        // should SaveState be based on scenes? worlds?
        /*
        if (st.load_from_file("path").has_value()) {
             // do something with loaded values from file
        } else {
             // create necessary default vals
        }
        */
        //somehow works lmao
        components::Transform t;
        components::Rigidbody r;
        t.setPosition({1.f, 5.f});
        r.velocity.x = 2.f;
        r.velocity.y = 11.f;
        auto e = st.bind_scoped("trs", t);
        auto er = st.bind_scoped("rgb", r);
        std::cout << st.dump() << std::endl;
        t.move({1.f, 0.4f});
        r.velocity += t.getPosition();
        std::cout << st.dump() << std::endl;
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

        loadLuaScript("scripts/lua/main.lua");


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

    void Game::loadLuaScript(const std::string& path) {
        const std::filesystem::path scriptPath(path);
        if (!std::filesystem::is_regular_file(scriptPath)) {
            throw std::runtime_error("Lua script does not exist: " + path);
        }
        const auto normalizedPath = std::filesystem::weakly_canonical(scriptPath).string();
        if (loadedLuaScripts.contains(normalizedPath)) return;
        try {
            lua.script_file(normalizedPath);
        } catch (const sol::error& error) {
            throw std::runtime_error("Failed to load Lua script '" + path + "': " + error.what());
        }
        loadedLuaScripts.insert(normalizedPath);
    }
} // ms