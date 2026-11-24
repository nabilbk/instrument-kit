/* SPDX-License-Identifier: MIT - the two showcase screens, shared by the headless and SDL hosts. */
#ifndef IDEV_DEMOS_H
#define IDEV_DEMOS_H

#include "idev_lvgl.h"

#define IDEV_DEMO_W 800
#define IDEV_DEMO_H 480

typedef struct {
    lv_obj_t *speed, *rpm, *coolant, *fuel;
    lv_obj_t *ann_oil, *ann_temp, *ann_fuel, *ann_brake, *ann_limit;
    idev_instrument_t oil; /* not displayed, drives the OIL light */
} idev_car_demo_t;

typedef struct {
    lv_obj_t *attitude, *airspeed, *altitude;
    lv_obj_t *ann_stall, *ann_bank, *ann_overspeed, *ann_ap;
} idev_pfd_demo_t;

void idev_car_demo_create(lv_obj_t *screen, idev_car_demo_t *demo);
/* invalid: feed NaN to show the "sensor lost" state. */
void idev_car_demo_update(idev_car_demo_t *demo, float t_s, bool invalid);

void idev_pfd_demo_create(lv_obj_t *screen, idev_pfd_demo_t *demo);
void idev_pfd_demo_update(idev_pfd_demo_t *demo, float t_s, bool invalid);

#endif
