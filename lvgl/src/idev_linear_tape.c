/* SPDX-License-Identifier: MIT - LinearTape: vertical moving scale (airspeed, altitude). */
#include "idev_lv_private.h"

#include <math.h>

#define HEADER_H 26
#define ZONE_W 6
#define MAJOR_LEN 14
#define MINOR_LEN 7
#define BOX_H 34

static void draw_tape(lv_obj_t *obj, idev_lv_data_t *d, lv_layer_t *layer)
{
    const idev_instrument_t *in = &d->axis[0];
    const idev_instrument_cfg_t *cfg = in->cfg;
    const idev_tape_params_t *tp = &cfg->tape;
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const int32_t w = lv_area_get_width(&a);

    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_init(&bg);
    bg.bg_color = idev_lv_color(idev_palette.surface);
    bg.border_color = idev_lv_color(idev_palette.surface_raised);
    bg.border_width = 2;
    bg.radius = 4;
    lv_draw_rect(layer, &bg, &a);

    char header[32];
    lv_snprintf(header, sizeof header, "%s %s", cfg->label, cfg->unit);
    idev_lv_text(layer, header, &lv_font_montserrat_14, idev_palette.dim,
                 a.x1 + w / 2, a.y1 + HEADER_H / 2, w);

    lv_area_t scale = {a.x1 + 2, a.y1 + HEADER_H, a.x2 - 2, a.y2 - 2};
    const int32_t sh = lv_area_get_height(&scale);
    const int32_t yc = scale.y1 + sh / 2;
    const float ppu = (float)sh / tp->visible_range;
    const float shown = in->valid ? in->display : cfg->min;
    /* Tick edge: right for the left-hand tape, left when mirrored. */
    const int32_t edge = d->mirrored ? scale.x1 : scale.x2;
    const int32_t dir = d->mirrored ? 1 : -1; /* from edge toward the labels */

    lv_area_t clip_saved = layer->_clip_area;
    lv_area_t clip;
    if (!idev_lv_area_intersect(&clip, &clip_saved, &scale)) {
        return;
    }
    layer->_clip_area = clip;

    if (in->valid) {
        for (size_t i = 0; i < cfg->zone_count; ++i) {
            const idev_zone_t *z = &cfg->zones[i];
            lv_area_t zr;
            zr.y1 = yc - (int32_t)lroundf((z->to - shown) * ppu);
            zr.y2 = yc - (int32_t)lroundf((z->from - shown) * ppu);
            zr.x1 = d->mirrored ? edge : edge - ZONE_W;
            zr.x2 = d->mirrored ? edge + ZONE_W : edge;
            lv_draw_rect_dsc_t zd;
            lv_draw_rect_dsc_init(&zd);
            zd.bg_color = idev_lv_color(idev_level_color(z->severity));
            lv_draw_rect(layer, &zd, &zr);
        }

        const int div = tp->minor_divisions < 1 ? 1 : tp->minor_divisions;
        const float minor = tp->major_step / (float)div;
        const float half = tp->visible_range / 2.0f + minor;
        const long first = (long)floorf((shown - half) / minor);
        const long last = (long)ceilf((shown + half) / minor);
        const int32_t tick_x0 = edge + dir * (ZONE_W + 1);
        for (long i = first; i <= last; ++i) {
            const float v = (float)i * minor;
            if (v < cfg->min - 1e-3f || v > cfg->max + 1e-3f) {
                continue;
            }
            const int32_t y = yc - (int32_t)lroundf((v - shown) * ppu);
            const bool major = (i % div) == 0;
            const int32_t len = major ? MAJOR_LEN : MINOR_LEN;
            lv_point_precise_t p0 = {tick_x0, y};
            lv_point_precise_t p1 = {tick_x0 + dir * len, y};
            idev_lv_line(layer, p0, p1, 2, major ? idev_palette.foreground : idev_palette.dim,
                         false);
            const float q = v / tp->label_step;
            /* No label half-hidden behind the readout box. */
            if (fabsf(q - roundf(q)) < 1e-3f && LV_ABS(y - yc) > BOX_H / 2 + 8) {
                char buf[16];
                lv_snprintf(buf, sizeof buf, "%ld", (long)lroundf(v));
                const int32_t lw = w - ZONE_W - MAJOR_LEN - 8;
                const int32_t lcx = tick_x0 + dir * (MAJOR_LEN + 2) + dir * lw / 2;
                idev_lv_text(layer, buf, &lv_font_montserrat_16, idev_palette.foreground, lcx, y,
                             lw);
            }
        }
    }

    /* Current value box with a pointer toward the tick edge */
    const uint32_t accent =
        in->alert == IDEV_LEVEL_NORMAL ? idev_palette.foreground : idev_level_color(in->alert);
    lv_area_t box = {scale.x1 + 4, yc - BOX_H / 2, scale.x2 - 12, yc + BOX_H / 2};
    if (d->mirrored) {
        box.x1 = scale.x1 + 12;
        box.x2 = scale.x2 - 4;
    }
    lv_draw_rect_dsc_t bd;
    lv_draw_rect_dsc_init(&bd);
    bd.bg_color = idev_lv_color(idev_palette.background);
    bd.border_color = idev_lv_color(accent);
    bd.border_width = 2;
    bd.radius = 3;
    lv_draw_rect(layer, &bd, &box);
    const int32_t px = d->mirrored ? box.x1 : box.x2;
    lv_point_precise_t t0 = {px, yc - 8};
    lv_point_precise_t t1 = {px, yc + 8};
    lv_point_precise_t t2 = {px - dir * 10, yc};
    idev_lv_triangle(layer, t0, t1, t2, accent);

    char value[16];
    idev_format(cfg, in->display, in->valid, value, sizeof value);
    idev_lv_text(layer, value, &lv_font_montserrat_20, in->valid ? accent : idev_palette.warning,
                 (box.x1 + box.x2) / 2, yc, lv_area_get_width(&box));

    layer->_clip_area = clip_saved;
}

lv_obj_t *idev_linear_tape_create(lv_obj_t *parent, const char *config_id, bool mirrored)
{
    const idev_instrument_cfg_t *cfg = idev_cfg_find(config_id);
    if (cfg == NULL || cfg->kind != IDEV_KIND_LINEAR_TAPE) {
        return NULL;
    }
    lv_obj_t *obj = idev_lv_create(parent, IDEV_KIND_LINEAR_TAPE, draw_tape);
    if (obj != NULL) {
        idev_lv_data_t *d = idev_lv_data(obj);
        idev_instrument_init(&d->axis[0], cfg);
        d->mirrored = mirrored;
        lv_obj_set_size(obj, 96, 360);
    }
    return obj;
}
