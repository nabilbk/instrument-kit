/* SPDX-License-Identifier: MIT - Annunciator: alert light, normal / caution / warning. */
#include "idev_lv_private.h"

static uint32_t level_rgb(const idev_lv_data_t *d)
{
    return idev_level_color(d->level);
}

static void draw_annunciator(lv_obj_t *obj, idev_lv_data_t *d, lv_layer_t *layer)
{
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const bool off = d->level == IDEV_LEVEL_NORMAL && !d->active;

    lv_draw_rect_dsc_t r;
    lv_draw_rect_dsc_init(&r);
    r.radius = 4;
    r.border_width = 2;
    uint32_t text_rgb;
    if (d->lit) {
        r.bg_color = idev_lv_color(level_rgb(d));
        r.border_color = r.bg_color;
        text_rgb = idev_palette.background;
    } else {
        r.bg_color = idev_lv_color(idev_palette.surface);
        r.border_color = idev_lv_color(off ? idev_palette.surface_raised : level_rgb(d));
        text_rgb = off ? idev_palette.dim : level_rgb(d);
    }
    lv_draw_rect(layer, &r, &a);
    idev_lv_text(layer, d->text, &lv_font_montserrat_16, text_rgb, (a.x1 + a.x2) / 2,
                 (a.y1 + a.y2) / 2, lv_area_get_width(&a));
}

static void refresh(lv_obj_t *obj, idev_lv_data_t *d)
{
    const bool lit = idev_annunciator_lit(d->level, d->active, d->acknowledged, lv_tick_get(),
                                          d->axis[0].cfg->annunciator.blink_hz);
    if (lit != d->lit) {
        d->lit = lit;
        lv_obj_invalidate(obj);
    }
    if (d->level == IDEV_LEVEL_WARNING && !d->acknowledged) {
        idev_lv_wake(obj);
    }
}

lv_obj_t *idev_annunciator_create(lv_obj_t *parent, const char *config_id)
{
    const idev_instrument_cfg_t *cfg = idev_cfg_find(config_id);
    if (cfg == NULL || cfg->kind != IDEV_KIND_ANNUNCIATOR) {
        return NULL;
    }
    lv_obj_t *obj = idev_lv_create(parent, IDEV_KIND_ANNUNCIATOR, draw_annunciator);
    if (obj != NULL) {
        idev_lv_data_t *d = idev_lv_data(obj);
        idev_instrument_init(&d->axis[0], cfg);
        d->text = cfg->label;
        d->level = IDEV_LEVEL_NORMAL;
        lv_obj_set_size(obj, 120, 40);
        lv_timer_set_period(d->timer, 50);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    return obj;
}

void idev_annunciator_set_level(lv_obj_t *obj, idev_level_t level, bool active)
{
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d == NULL || d->kind != IDEV_KIND_ANNUNCIATOR) {
        return;
    }
    if (level != d->level || active != d->active) {
        if (level > d->level) {
            d->acknowledged = false; /* a new, higher alert must be acknowledged again */
        }
        d->level = level;
        d->active = active;
        lv_obj_invalidate(obj);
    }
    refresh(obj, d);
}

void idev_annunciator_acknowledge(lv_obj_t *obj)
{
    idev_lv_data_t *d = idev_lv_data(obj);
    if (d != NULL && d->kind == IDEV_KIND_ANNUNCIATOR) {
        d->acknowledged = true;
        refresh(obj, d);
        lv_obj_invalidate(obj);
    }
}

bool idev_annunciator_is_lit(lv_obj_t *obj)
{
    const idev_lv_data_t *d = idev_lv_data(obj);
    return d != NULL && d->lit;
}
