/* SPDX-License-Identifier: MIT - AttitudeIndicator: artificial horizon (roll / pitch). */
#include "idev_lv_private.h"

#include <math.h>

#define IDEV_DEG2RAD 0.017453292519943f

typedef struct {
    float cx, cy;   /* view centre, screen px */
    float c, s;     /* cos/sin of the horizon rotation (-roll) */
    float offset;   /* pitch offset along the horizon normal, px */
} frame_t;

/* Horizon frame (x right, y down, origin at zero pitch) to screen. */
static lv_point_precise_t to_screen(const frame_t *f, float lx, float ly)
{
    const float y = ly + f->offset;
    lv_point_precise_t p;
    p.x = (lv_value_precise_t)lroundf(f->cx + lx * f->c - y * f->s);
    p.y = (lv_value_precise_t)lroundf(f->cy + lx * f->s + y * f->c);
    return p;
}

static void draw_roll_scale(lv_layer_t *layer, const frame_t *f, float radius, uint32_t pointer_rgb)
{
    static const int marks[] = {-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60};
    const int32_t cx = (int32_t)f->cx;
    const int32_t cy = (int32_t)f->cy;

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.center.x = cx;
    arc.center.y = cy;
    arc.radius = (uint16_t)(radius + 1.0f);
    arc.width = 2;
    arc.start_angle = idev_lv_arc_angle(-60.0f);
    arc.end_angle = idev_lv_arc_angle(60.0f);
    arc.color = idev_lv_color(idev_palette.foreground);
    lv_draw_arc(layer, &arc);

    for (unsigned i = 0; i < sizeof marks / sizeof marks[0]; ++i) {
        const int m = marks[i];
        const float len = (m == 0 || m == 30 || m == -30 || m == 60 || m == -60) ? 14.0f : 8.0f;
        if (m == 0) {
            idev_lv_triangle(layer, idev_lv_polar(cx, cy, radius + 2.0f, 0.0f),
                             idev_lv_polar(cx, cy, radius + 14.0f, -3.5f),
                             idev_lv_polar(cx, cy, radius + 14.0f, 3.5f),
                             idev_palette.foreground);
            continue;
        }
        idev_lv_line(layer, idev_lv_polar(cx, cy, radius, (float)m),
                     idev_lv_polar(cx, cy, radius + len, (float)m), 2, idev_palette.foreground,
                     false);
    }

    /* Sky pointer: turns with the horizon. */
    const lv_point_precise_t tip = to_screen(&(frame_t){f->cx, f->cy, f->c, f->s, 0.0f}, 0.0f,
                                             -radius + 2.0f);
    const lv_point_precise_t b1 = to_screen(&(frame_t){f->cx, f->cy, f->c, f->s, 0.0f}, -9.0f,
                                            -radius + 17.0f);
    const lv_point_precise_t b2 = to_screen(&(frame_t){f->cx, f->cy, f->c, f->s, 0.0f}, 9.0f,
                                            -radius + 17.0f);
    idev_lv_triangle(layer, tip, b1, b2, pointer_rgb);
}

static void draw_aircraft(lv_layer_t *layer, int32_t cx, int32_t cy, int32_t w)
{
    const int32_t outer = w * 30 / 100;
    const int32_t inner = w * 10 / 100;
    const int32_t drop = w * 4 / 100;
    for (int pass = 0; pass < 2; ++pass) {
        const int32_t width = pass == 0 ? 9 : 5;
        const uint32_t rgb = pass == 0 ? 0x000000u : idev_palette.symbol;
        for (int side = -1; side <= 1; side += 2) {
            lv_point_precise_t a = {cx + side * outer, cy};
            lv_point_precise_t b = {cx + side * inner, cy};
            lv_point_precise_t c = {cx + side * inner, cy + drop};
            idev_lv_line(layer, a, b, width, rgb, false);
            idev_lv_line(layer, b, c, width, rgb, false);
        }
        lv_draw_rect_dsc_t dot;
        lv_draw_rect_dsc_init(&dot);
        dot.bg_color = idev_lv_color(rgb);
        const int32_t r = pass == 0 ? 6 : 4;
        lv_area_t da = {cx - r, cy - r, cx + r, cy + r};
        lv_draw_rect(layer, &dot, &da);
    }
}

static void draw_attitude(lv_obj_t *obj, idev_lv_data_t *d, lv_layer_t *layer)
{
    const idev_instrument_t *roll = &d->axis[0];
    const idev_instrument_t *pitch = &d->axis[1];
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const int32_t w = lv_area_get_width(&a);
    const int32_t h = lv_area_get_height(&a);
    const int32_t cx = a.x1 + w / 2;
    const int32_t cy = a.y1 + h / 2;

    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_init(&bg);
    bg.radius = 8;

    if (!roll->valid || !pitch->valid) {
        bg.bg_color = idev_lv_color(idev_palette.surface);
        bg.border_color = idev_lv_color(idev_palette.surface_raised);
        bg.border_width = 2;
        lv_draw_rect(layer, &bg, &a);
        lv_draw_rect_dsc_t flag;
        lv_draw_rect_dsc_init(&flag);
        flag.bg_color = idev_lv_color(idev_palette.warning);
        flag.radius = 3;
        lv_area_t fa = {cx - 40, cy - 20, cx + 40, cy + 20};
        lv_draw_rect(layer, &flag, &fa);
        idev_lv_text(layer, "ATT", &lv_font_montserrat_28, idev_palette.background, cx, cy, 80);
        return;
    }

    bg.bg_color = idev_lv_color(idev_palette.sky);
    lv_draw_rect(layer, &bg, &a);

    const float ppd = (float)h / pitch->cfg->attitude.pitch_visible_range;
    const float th = -roll->display * IDEV_DEG2RAD;
    const frame_t f = {(float)cx, (float)cy, cosf(th), sinf(th), pitch->display * ppd};
    /* The view fits in a circle of radius `half` around its centre. In the horizon frame that
     * circle is centred on (0, -offset). Keep geometry within it: very large coordinates
     * overflow LVGL's software line masks (found with UBSan). */
    const float half = sqrtf((float)(w * w + h * h)) / 2.0f + 2.0f;
    if (f.offset < -half) {
        lv_draw_rect_dsc_t g;
        lv_draw_rect_dsc_init(&g);
        g.bg_color = idev_lv_color(idev_palette.ground);
        g.radius = 8;
        lv_draw_rect(layer, &g, &a); /* horizon above the view: all ground */
    } else if (f.offset <= half) {
        /* One triangle (base on the horizon, apex below) covers the ground part of the view
         * circle: no seam between two triangles. Its sides stay >= `half` away from the
         * circle centre for any offset in [-half, half]. */
        const lv_point_precise_t b0 = to_screen(&f, -3.0f * half, 0.0f);
        const lv_point_precise_t b1 = to_screen(&f, 3.0f * half, 0.0f);
        const lv_point_precise_t apex = to_screen(&f, 0.0f, 3.0f * half);
        idev_lv_triangle(layer, b0, b1, apex, idev_palette.ground);
        idev_lv_line(layer, to_screen(&f, -half, 0.0f), to_screen(&f, half, 0.0f), 2,
                     idev_palette.foreground, false);
    } /* else horizon below the view: all sky */

    /* Pitch ladder */
    const float step = pitch->cfg->attitude.ladder_step > 0.0f ? pitch->cfg->attitude.ladder_step : 5.0f;
    const float roll_r = (float)LV_MIN(w, h) * 0.40f;
    /* Rungs stay inside the roll scale so they never cross it. */
    const float ladder_reach = roll_r * 0.8f;
    for (float p = -90.0f + step; p < 90.0f; p += step) {
        if (fabsf(p) < 0.01f || fabsf(p - pitch->display) * ppd > ladder_reach) {
            continue;
        }
        const bool major = fabsf(fmodf(p, 10.0f)) < 0.01f;
        const float hw = (float)w * (major ? 0.14f : 0.06f);
        const float ly = -p * ppd;
        idev_lv_line(layer, to_screen(&f, -hw, ly), to_screen(&f, hw, ly), 2,
                     idev_palette.foreground, false);
        if (major) {
            char buf[8];
            lv_snprintf(buf, sizeof buf, "%d", (int)lroundf(fabsf(p)));
            for (int side = -1; side <= 1; side += 2) {
                const lv_point_precise_t t = to_screen(&f, (float)side * (hw + 18.0f), ly);
                idev_lv_text(layer, buf, &lv_font_montserrat_14, idev_palette.foreground,
                             (int32_t)t.x, (int32_t)t.y, 32);
            }
        }
    }

    const uint32_t pointer_rgb =
        roll->alert == IDEV_LEVEL_NORMAL ? idev_palette.foreground : idev_level_color(roll->alert);
    draw_roll_scale(layer, &f, roll_r, pointer_rgb);
    draw_aircraft(layer, cx, cy, w);
}

lv_obj_t *idev_attitude_create(lv_obj_t *parent, const char *roll_id, const char *pitch_id)
{
    const idev_instrument_cfg_t *rc = idev_cfg_find(roll_id);
    const idev_instrument_cfg_t *pc = idev_cfg_find(pitch_id);
    if (rc == NULL || pc == NULL || rc->kind != IDEV_KIND_ATTITUDE_AXIS ||
        pc->kind != IDEV_KIND_ATTITUDE_AXIS) {
        return NULL;
    }
    lv_obj_t *obj = idev_lv_create(parent, IDEV_KIND_ATTITUDE_AXIS, draw_attitude);
    if (obj != NULL) {
        idev_lv_data_t *d = idev_lv_data(obj);
        idev_instrument_init(&d->axis[0], rc);
        idev_instrument_init(&d->axis[1], pc);
        lv_obj_set_size(obj, 360, 360);
    }
    return obj;
}

void idev_attitude_set(lv_obj_t *obj, float roll_deg, float pitch_deg)
{
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d == NULL || d->kind != IDEV_KIND_ATTITUDE_AXIS) {
        return;
    }
    const bool was_valid = d->axis[0].valid && d->axis[1].valid;
    const idev_level_t was_alert = d->axis[0].alert;
    idev_instrument_set_value(&d->axis[0], roll_deg);
    idev_instrument_set_value(&d->axis[1], pitch_deg);
    if (was_valid != (d->axis[0].valid && d->axis[1].valid) || was_alert != d->axis[0].alert) {
        lv_obj_invalidate(obj);
    }
    idev_lv_wake(obj);
}
