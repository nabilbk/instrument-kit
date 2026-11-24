/* SPDX-License-Identifier: MIT
 * Interactive PC simulator using LVGL's SDL2 driver (needs SDL2 development files).
 *   idev_lvgl_sdl car|pfd
 * Click an annunciator to acknowledge a flashing warning. */
#include "idev_demos.h"

#include <SDL2/SDL.h>
#include <string.h>

int main(int argc, char **argv)
{
    const bool pfd = argc > 1 && strcmp(argv[1], "pfd") == 0;
    lv_init();
    lv_display_t *disp = lv_sdl_window_create(IDEV_DEMO_W, IDEV_DEMO_H);
    lv_sdl_mouse_create();
    (void)disp;

    idev_car_demo_t car;
    idev_pfd_demo_t pfdd;
    if (pfd) {
        idev_pfd_demo_create(lv_screen_active(), &pfdd);
    } else {
        idev_car_demo_create(lv_screen_active(), &car);
    }
    const uint32_t start = SDL_GetTicks();
    for (;;) {
        const float t = (float)(SDL_GetTicks() - start) / 1000.0f;
        if (pfd) {
            idev_pfd_demo_update(&pfdd, t, false);
        } else {
            idev_car_demo_update(&car, t, false);
        }
        const uint32_t wait = lv_timer_handler();
        SDL_Delay(wait < 5 ? wait : 5);
    }
    return 0;
}
