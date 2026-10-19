//
// Created by iformat on 6/2/26.
//

#include "Game.h"
#include <nuklear.h>
#include "nuklear_sfml_gl3.h"
#include <glad/glad.h>

#include "Components/Drawable.h"
#include "Components/Rigidbody.h"
#include "Components/Transform.h"
#include "Components/Player.h"
#include "Components/ShootingAccess.h""
#include "Events/EventDispatcher.h"
#include "Systems/SpatialHashing/Quadtree.h"


namespace ms
{
    Game::Game(): worldBox(),
                  settings(24, 8, 4, 3, 3),
                  mainWindow(sf::VideoMode({800, 600}), "My window", sf::Style::Default, sf::State::Windowed, settings),
                  eventHandler(*this),
                  sceneManager(worldBox.getWorlds()) {
        mainWindow.setFramerateLimit(60);
        mainWindow.setVerticalSyncEnabled(true);
        mainWindow.setActive(true);

        if(!gladLoadGL()) { /* Load OpenGL extensions */
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

        while (mainWindow.isOpen()) {
            sf::Time elapsed = clock.restart();

            handleEvents();

            //TODO: make c++ bindings to nuklear ui
            //make ui class that can store ui context
            {
                nk_style *s = &ctx->style;
                nk_style_push_color(ctx, &s->window.background, nk_rgba(0,0,0,0));
                nk_style_push_style_item(ctx, &s->window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,0)));
                if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250), NK_WINDOW_DYNAMIC))
                {
                    nk_layout_row_static(ctx, 30, 80, 1);
                    if (nk_button_label(ctx, "button")) {
                        std::cout << "button pressed \n";
                    }
                }
                nk_end(ctx);
                nk_style_pop_color(ctx);
                nk_style_pop_style_item(ctx);
            }

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