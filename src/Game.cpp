//
// Created by iformat on 6/2/26.
//

#include "Game.h"
#include <nuklear.h>
#include "nuklear_sfml_gl3.h"
#include <glad/glad.h>

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

    auto Game::getMainWindow() -> sf::RenderWindow & {
        return mainWindow;
    }

    void Game::update(sf::Time const &elapsed) {
        sceneManager.updateCurrentScene(elapsed.asSeconds());
    }

    void Game::handleEvents(nk_context *ctx) {
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

        if(!gladLoadGL()) { /* Load OpenGL extensions */
            printf("Failed to load OpenGL extensions!\n");
            return;
        }
        glViewport(0, 0, mainWindow.getSize().x, mainWindow.getSize().y);

        struct nk_context* ctx;
        ctx = nk_sfml_init(&mainWindow);
        struct nk_font_atlas *atlas;
        nk_sfml_font_stash_begin(&atlas);
        nk_sfml_font_stash_end();

        struct nk_colorf bg;
        bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

        while (mainWindow.isOpen()) {
            sf::Time elapsed = clock.restart();

            handleEvents(ctx);

            //TODO: make c++ bindings to nuklear ui
            //make ui class that can store ui context
            if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
                         NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                         NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
            {
                enum {EASY, HARD};
                static int op = EASY;
                static int property = 20;
                nk_layout_row_static(ctx, 30, 80, 1);
                if (nk_button_label(ctx, "button"))
                    fprintf(stdout, "button pressed\n");

                nk_layout_row_dynamic(ctx, 30, 2);
                if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
                if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

                nk_layout_row_dynamic(ctx, 25, 1);
                nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, "background:", NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 25, 1);
                if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
                    nk_layout_row_dynamic(ctx, 120, 1);
                    bg = nk_color_picker(ctx, bg, NK_RGBA);
                    nk_layout_row_dynamic(ctx, 25, 1);
                    bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                    bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                    bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                    bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
                    nk_combo_end(ctx);
                }
            }
            nk_end(ctx);

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