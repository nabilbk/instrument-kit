/* SPDX-License-Identifier: MIT - RoundGauge: needle dial with coloured zones. */
#include "idev_lv_private.h"

#include <math.h>

static float angle_of(const idev_instrument_cfg_t *cfg, float value)
{
    return cfg->gauge.start_angle + idev_ratio(cfg, value) * cfg->gauge.sweep_angle;
}

static void draw_gauge(lv_obj_t *obj, idev_lv_data_t *d, lv_layer_t *layer)
{
    const idev_instrument_t *in = &d->axis[0];
    const idev_instrument_cfg_t *cfg = in->cfg;
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const int32_t size = LV_MIN(lv_area_get_width(&a), lv_area_get_height(&a));
    const int32_t cx = a.x1 + lv_area_get_width(&a) / 2;
    const int32_t cy = a.y1 + lv_area_get_height(&a) / 2;
    const float r = (float)size / 2.0f - 2.0f;
    const bool large = size >= 240;

    /* Dial face */
    lv_draw_rect_dsc_t face;
    lv_draw_rect_dsc_init(&face);
    face.bg_color = idev_lv_color(idev_palette.surface);
    face.radius = LV_RADIUS_CIRCLE;
    face.border_color = idev_lv_color(idev_palette.surface_raised);
    face.border_width = LV_MAX(2, size / 80);
    lv_area_t fa = {cx - (int32_t)r, cy - (int32_t)r, cx + (int32_t)r, cy + (int32_t)r};
    lv_draw_rect(layer, &face, &fa);

    /* Zones on the outer ring */
    const float zone_w = LV_MAX(4.0f, (float)size * 0.035f);
    const float zone_r = r - (float)face.border_width - 3.0f;
    for (size_t i = 0; i < cfg->zone_count; ++i) {
        const idev_zone_t *z = &cfg->zones[i];
        lv_draw_arc_dsc_t arc;
        lv_draw_arc_dsc_init(&arc);
        arc.center.x = cx;
        arc.center.y = cy;
        arc.radius = (uint16_t)zone_r;
        arc.width = (int32_t)zone_w;
        arc.start_angle = idev_lv_arc_angle(angle_of(cfg, z->from));
        arc.end_angle = idev_lv_arc_angle(angle_of(cfg, z->to));
        arc.color = idev_lv_color(idev_level_color(z->severity));
        lv_draw_arc(layer, &arc);
    }

    /* Ticks and labels */
    const float tick_outer = zone_r - zone_w - 3.0f;
    const float major_len = (float)size * 0.075f;
    const float minor_len = (float)size * 0.035f;
    const int minor_div = cfg->gauge.minor_divisions < 1 ? 1 : cfg->gauge.minor_divisions;
    const float minor_step = cfg->gauge.major_step / (float)minor_div;
    const int ticks = (int)floorf((cfg->max - cfg->min) / minor_step + 0.001f);
    const lv_font_t *label_font = large ? &lv_font_montserrat_20 : &lv_font_montserrat_12;
    const float label_r = tick_outer - major_len - (float)lv_font_get_line_height(label_font) * 0.75f;
    for (int i = 0; i <= ticks; ++i) {
        const float v = cfg->min + (float)i * minor_step;
        const float ang = angle_of(cfg, v);
        const bool major = (i % minor_div) == 0;
        const float len = major ? major_len : minor_len;
        idev_lv_line(layer, idev_lv_polar(cx, cy, tick_outer, ang),
                     idev_lv_polar(cx, cy, tick_outer - len, ang),
                     major ? LV_MAX(2, size / 90) : LV_MAX(1, size / 180),
                     major ? idev_palette.foreground : idev_palette.dim, false);
        if (major) {
            char buf[12];
            idev_format_scale_label(cfg, v, buf, sizeof buf);
            const lv_point_precise_t p = idev_lv_polar(cx, cy, label_r, ang);
            idev_lv_text(layer, buf, label_font, idev_palette.foreground, (int32_t)p.x,
                         (int32_t)p.y, 60);
        }
    }

    /* Texts */
    const lv_font_t *small = large ? &lv_font_montserrat_16 : &lv_font_montserrat_12;
    const lv_font_t *value_font = large ? &lv_font_montserrat_40 : &lv_font_montserrat_20;
    char caption[32];
    char value[16];
    idev_format(cfg, in->display, in->valid, value, sizeof value);
    const uint32_t value_rgb =
        in->alert == IDEV_LEVEL_NORMAL ? idev_palette.foreground : idev_level_color(in->alert);
    if (large) {
        lv_snprintf(caption, sizeof caption, "%s%s%s", cfg->label,
                    cfg->gauge.scale_caption ? " " : "",
                    cfg->gauge.scale_caption ? cfg->gauge.scale_caption : "");
        idev_lv_text(layer, caption, small, idev_palette.dim, cx, cy - (int32_t)(r * 0.30f), size);
        idev_lv_text(layer, value, value_font, value_rgb, cx, cy + (int32_t)(r * 0.42f), size);
        idev_lv_text(layer, cfg->unit, small, idev_palette.dim, cx, cy + (int32_t)(r * 0.66f),
                     size);
    } else {
        lv_snprintf(caption, sizeof caption, "%s %s", cfg->label, cfg->unit);
        idev_lv_text(layer, value, value_font, value_rgb, cx, cy + (int32_t)(r * 0.45f), size);
        idev_lv_text(layer, caption, small, idev_palette.dim, cx, cy + (int32_t)(r * 0.80f), size);
    }

    /* Needle (hidden when data is invalid) */
    if (in->valid) {
        const float ang = angle_of(cfg, in->display);
        idev_lv_line(layer, idev_lv_polar(cx, cy, -(float)size * 0.06f, ang),
                     idev_lv_polar(cx, cy, tick_outer - 2.0f, ang), LV_MAX(3, size / 55),
                     idev_palette.needle, true);
    }
    lv_draw_rect_dsc_t hub;
    lv_draw_rect_dsc_init(&hub);
    hub.bg_color = idev_lv_color(idev_palette.surface_raised);
    hub.radius = LV_RADIUS_CIRCLE;
    hub.border_color = idev_lv_color(idev_palette.dim);
    hub.border_width = 2;
    const int32_t hr = LV_MAX(5, size / 22);
    lv_area_t ha = {cx - hr, cy - hr, cx + hr, cy + hr};
    lv_draw_rect(layer, &hub, &ha);
}

lv_obj_t *idev_round_gauge_create(lv_obj_t *parent, const char *config_id)
{
    const idev_instrument_cfg_t *cfg = idev_cfg_find(config_id);
    if (cfg == NULL || cfg->kind != IDEV_KIND_ROUND_GAUGE) {
        return NULL;
    }
    lv_obj_t *obj = idev_lv_create(parent, IDEV_KIND_ROUND_GAUGE, draw_gauge);
    if (obj != NULL) {
        idev_instrument_init(&idev_lv_data(obj)->axis[0], cfg);
        lv_obj_set_size(obj, 240, 240);
    }
    return obj;
}
