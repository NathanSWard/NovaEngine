#include <sdl2pp/init.hpp>
#include <sdl2pp/window.hpp>
#include <sdl2pp/renderer.hpp>
#include <sdl2pp/event.hpp>
#include <sdl2pp/color.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <entt/entt.hpp>

#include <iostream>

using namespace sdl2;

struct pos { float x = 0.f; float y = 0.f; };
struct vel { float dx = 0.f; float dy = 0.f; };

int main(int, char**) {
    SDL2 sdl(sdl2_init_flags::EVERYTHING);
    if (!sdl) {
        spdlog::error("SDL2 Err: {}\n", SDL2::get_error());
        return EXIT_FAILURE;
    }

    IMG img(img_init_flags::ALL);
    if (!img) {
        spdlog::error("IMG Err: {}\n", IMG::get_error());
        return EXIT_FAILURE;
    }

    window win("Game", window::pos_centered, {800, 800}, window_flags::NONE);
    if (!win) {
        spdlog::error("SDL2 Window Err: {}\n", SDL2::get_error());
        return EXIT_FAILURE;
    }

    renderer ren(win, renderer_flags::ACCELERATED);
    if (!ren) {
        spdlog::error("SDL2 Renderer Err: {}\n", SDL2::get_error());
        return EXIT_FAILURE;
    }

    entt::registry reg;

    for (int i = 0; i < 10; ++i) {
        auto e = reg.create();
        reg.emplace<pos>(e, 5.f * i, 5.f * i);
        reg.emplace<vel>(e, 10.f, 10.f);
    }

    bool quit = false;
    auto time = std::chrono::high_resolution_clock::now();
    while (!quit) {
        for (auto const& event : event_queue) {
            if (event.type == SDL_QUIT)
                quit = true;
        }
        ren.set_draw_color(colors::black);
        ren.clear();
        auto const now = std::chrono::high_resolution_clock::now();
        auto const diff = now - time;
        time = now; 
        auto const g = reg.group<pos, vel const>();
        g.each([diff = std::chrono::round<std::chrono::seconds>(diff), &ren](pos& p, vel const& v) {
            p.x += v.dy * diff.count();
            p.y += v.dy * diff.count(); 
        });

        ren.set_draw_color(colors::red);
        auto const view = reg.view<pos const>();
        view.each([&ren](auto const& p) {
            ren.draw_rect(rect<float>{p.x, p.y, 10.f, 10.f});
        });
        ren.present();
    }

    return EXIT_SUCCESS;
}
