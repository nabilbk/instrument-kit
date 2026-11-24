/* SPDX-License-Identifier: MIT */
#include "idev_lv_private.h"

#include <math.h>
#include <string.h>

#define IDEV_DEG2RAD 0.017453292519943f

static void timer_cb(lv_timer_t *timer)
{
    lv_obj_t *obj = lv_timer_get_user_data(timer);
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d == NULL) {
        return;
    }
    const uint32_t now = lv_tick_get();
    const float dt = (float)(uint32_t)(now - d->last_ms);
    d->last_ms = now;

    bool dirty = false;
    bool busy = false;
    if (d->kind == IDEV_KIND_ANNUNCIATOR) {
        const bool lit = idev_annunciator_lit(d->level, d->active, d->acknowledged, now,
                                              d->axis[0].cfg->annunciator.blink_hz);
        dirty = lit != d->lit;
        d->lit = lit;
        busy = d->level == IDEV_LEVEL_WARNING && !d->acknowledged;
    } else {
        for (int i = 0; i < 2; ++i) {
            if (d->axis[i].cfg != NULL) {
                dirty |= idev_instrument_step(&d->axis[i], dt);
                busy |= idev_instrument_settling(&d->axis[i]);
            }
        }
    }
    if (dirty) {
        lv_obj_invalidate(obj);
    }
    if (!busy) {
        lv_timer_pause(timer); /* no CPU while nothing moves */
    }
}

static void event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_current_target(e);
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d == NULL) {
        return;
    }
    switch (lv_event_get_code(e)) {
    case LV_EVENT_DRAW_MAIN: {
        idev_lv_draw_fn draw = (idev_lv_draw_fn)lv_event_get_user_data(e);
        draw(obj, d, lv_event_get_layer(e));
        break;
    }
    case LV_EVENT_CLICKED:
        if (d->kind == IDEV_KIND_ANNUNCIATOR) {
            idev_annunciator_acknowledge(obj);
        }
        break;
    case LV_EVENT_DELETE:
        lv_timer_delete(d->timer);
        d->magic = 0;
        lv_free(d);
        lv_obj_set_user_data(obj, NULL);
        break;
    default:
        break;
    }
}

lv_obj_t *idev_lv_create(lv_obj_t *parent, idev_kind_t kind, idev_lv_draw_fn draw)
{
    idev_lv_data_t *d = lv_malloc(sizeof(*d));
    if (d == NULL) {
        return NULL;
    }
    memset(d, 0, sizeof(*d));
    d->magic = IDEV_LV_MAGIC;
    d->kind = kind;

    lv_obj_t *obj = lv_obj_create(parent);
    if (obj == NULL) {
        lv_free(d);
        return NULL;
    }
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(obj, d);
    lv_obj_add_event_cb(obj, event_cb, LV_EVENT_ALL, (void *)draw);

    d->last_ms = lv_tick_get();
    d->timer = lv_timer_create(timer_cb, IDEV_LV_TICK_MS, obj);
    lv_timer_pause(d->timer);
    return obj;
}

idev_lv_data_t *idev_lv_data(lv_obj_t *obj)
{
    if (obj == NULL) {
        return NULL;
    }
    idev_lv_data_t *d = lv_obj_get_user_data(obj);
    return (d != NULL && d->magic == IDEV_LV_MAGIC) ? d : NULL;
}

void idev_lv_wake(lv_obj_t *obj)
{
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d != NULL && lv_timer_get_paused(d->timer)) {
        d->last_ms = lv_tick_get();
        lv_timer_resume(d->timer);
    }
}

const idev_instrument_t *idev_widget_instrument(lv_obj_t *obj, int axis)
{
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d == NULL || axis < 0 || axis > 1) {
        return NULL;
    }
    return &d->axis[axis];
}

void idev_instrument_widget_set_value(lv_obj_t *obj, float value)
{
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d == NULL || d->kind == IDEV_KIND_ANNUNCIATOR || d->axis[0].cfg == NULL) {
        return;
    }
    const bool was_valid = d->axis[0].valid;
    const idev_level_t was_alert = d->axis[0].alert;
    idev_instrument_set_value(&d->axis[0], value);
    if (was_valid != d->axis[0].valid || was_alert != d->axis[0].alert) {
        lv_obj_invalidate(obj);
    }
    idev_lv_wake(obj);
}

lv_point_precise_t idev_lv_polar(int32_t cx, int32_t cy, float radius, float angle_deg)
{
    const float a = angle_deg * IDEV_DEG2RAD;
    lv_point_precise_t p;
    p.x = (lv_value_precise_t)lroundf((float)cx + radius * sinf(a));
    p.y = (lv_value_precise_t)lroundf((float)cy - radius * cosf(a));
    return p;
}

int32_t idev_lv_arc_angle(float angle_deg)
{
    int32_t a = (int32_t)lroundf(angle_deg - 90.0f) % 360;
    return a < 0 ? a + 360 : a;
}

void idev_lv_line(lv_layer_t *layer, lv_point_precise_t a, lv_point_precise_t b, int32_t width,
                  uint32_t rgb, bool round)
{
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.p1 = a;
    dsc.p2 = b;
    dsc.width = width;
    dsc.color = idev_lv_color(rgb);
    dsc.round_start = round;
    dsc.round_end = round;
    lv_draw_line(layer, &dsc);
}

void idev_lv_text(lv_layer_t *layer, const char *text, const lv_font_t *font, uint32_t rgb,
                  int32_t cx, int32_t cy, int32_t width)
{
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text = text;
    dsc.font = font;
    dsc.color = idev_lv_color(rgb);
    dsc.align = LV_TEXT_ALIGN_CENTER;
    const int32_t h = lv_font_get_line_height(font);
    lv_area_t area = {cx - width / 2, cy - h / 2, cx + width / 2, cy + h / 2};
    lv_draw_label(layer, &dsc, &area);
}

void idev_lv_triangle(lv_layer_t *layer, lv_point_precise_t a, lv_point_precise_t b,
                      lv_point_precise_t c, uint32_t rgb)
{
    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.p[0] = a;
    dsc.p[1] = b;
    dsc.p[2] = c;
    dsc.bg_color = idev_lv_color(rgb);
    dsc.bg_opa = LV_OPA_COVER;
    lv_draw_triangle(layer, &dsc);
}
