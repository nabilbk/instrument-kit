/* SPDX-License-Identifier: MIT - car cluster demo: speed, RPM, coolant, fuel, warning lights. */
#include "idev_demos.h"
#include "idev/sim.h"

#include <math.h>

static lv_obj_t *place(lv_obj_t *obj, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    return obj;
}

void idev_car_demo_create(lv_obj_t *screen, idev_car_demo_t *demo)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(idev_palette.background), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    demo->speed = place(idev_round_gauge_create(screen, "car.speed"), 24, 40, 330, 330);
    demo->rpm = place(idev_round_gauge_create(screen, "car.rpm"), 446, 40, 330, 330);
    demo->coolant = place(idev_round_gauge_create(screen, "car.coolant"), 340, 70, 120, 120);
    demo->fuel = place(idev_round_gauge_create(screen, "car.fuel"), 340, 220, 120, 120);

    lv_obj_t **ann[] = {&demo->ann_oil, &demo->ann_temp, &demo->ann_fuel, &demo->ann_brake,
                        &demo->ann_limit};
    const char *ids[] = {"car.ann.oil", "car.ann.temp", "car.ann.fuel", "car.ann.brake",
                         "car.ann.speed"};
    for (int i = 0; i < 5; ++i) {
        *ann[i] = place(idev_annunciator_create(screen, ids[i]), 85 + i * 130, 410, 110, 40);
    }
    idev_instrument_init(&demo->oil, idev_cfg_find("car.oilPressure"));
}

void idev_car_demo_update(idev_car_demo_t *demo, float t_s, bool invalid)
{
    idev_car_frame_t f;
    idev_sim_car(t_s, &f);
    if (invalid) {
        f.rpm = NAN;
        f.coolant_c = NAN;
    }
    idev_instrument_widget_set_value(demo->speed, f.speed_kmh);
    idev_instrument_widget_set_value(demo->rpm, f.rpm);
    idev_instrument_widget_set_value(demo->coolant, f.coolant_c);
    idev_instrument_widget_set_value(demo->fuel, f.fuel_pct);
    idev_instrument_set_value(&demo->oil, f.oil_bar);

    idev_annunciator_set_level(demo->ann_oil, demo->oil.alert, false);
    idev_annunciator_set_level(demo->ann_temp, idev_widget_instrument(demo->coolant, 0)->alert,
                               false);
    idev_annunciator_set_level(demo->ann_fuel, idev_widget_instrument(demo->fuel, 0)->alert, false);
    idev_annunciator_set_level(demo->ann_brake, f.brake ? IDEV_LEVEL_CAUTION : IDEV_LEVEL_NORMAL,
                               false);
    idev_annunciator_set_level(demo->ann_limit, idev_widget_instrument(demo->speed, 0)->alert,
                               false);
}
