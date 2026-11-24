/* SPDX-License-Identifier: MIT - private helpers shared by the IDEV LVGL widgets. */
#ifndef IDEV_LV_PRIVATE_H
#define IDEV_LV_PRIVATE_H

#include "idev_lvgl.h"

/* LVGL 9.2 exposes the area intersection as _lv_area_intersect; 9.3+ as lv_area_intersect. */
#if LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR < 3
#define idev_lv_area_intersect _lv_area_intersect
#else
#define idev_lv_area_intersect lv_area_intersect
#endif

#define IDEV_LV_MAGIC 0x49444556u /* "IDEV" */
#define IDEV_LV_TICK_MS 16u

typedef struct {
    uint32_t magic;
    idev_kind_t kind;
    idev_instrument_t axis[2]; /* [0] value or roll, [1] pitch */
    lv_timer_t *timer;
    uint32_t last_ms;
    /* Linear tape */
    bool mirrored;
    /* Annunciator */
    const char *text;
    idev_level_t level;
    bool active;
    bool acknowledged;
    bool lit;
} idev_lv_data_t;

typedef void (*idev_lv_draw_fn)(lv_obj_t *obj, idev_lv_data_t *d, lv_layer_t *layer);

lv_obj_t *idev_lv_create(lv_obj_t *parent, idev_kind_t kind, idev_lv_draw_fn draw);
idev_lv_data_t *idev_lv_data(lv_obj_t *obj);
void idev_lv_wake(lv_obj_t *obj);

static inline lv_color_t idev_lv_color(uint32_t rgb)
{
    return lv_color_hex(rgb);
}

/* Geometry: angles in degrees, clockwise from 12 o'clock. */
lv_point_precise_t idev_lv_polar(int32_t cx, int32_t cy, float radius, float angle_deg);
int32_t idev_lv_arc_angle(float angle_deg); /* to LVGL arc convention (0 = 3 o'clock) */

void idev_lv_line(lv_layer_t *layer, lv_point_precise_t a, lv_point_precise_t b, int32_t width,
                  uint32_t rgb, bool round);
void idev_lv_text(lv_layer_t *layer, const char *text, const lv_font_t *font, uint32_t rgb,
                  int32_t cx, int32_t cy, int32_t width);
void idev_lv_triangle(lv_layer_t *layer, lv_point_precise_t a, lv_point_precise_t b,
                      lv_point_precise_t c, uint32_t rgb);

#endif
