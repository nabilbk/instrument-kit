/* SPDX-License-Identifier: MIT - primary flight display demo: horizon, speed and altitude tapes, alerts. */
#include "idev_demos.h"
#include "idev/sim.h"

#include <math.h>

static lv_obj_t *place(lv_obj_t *obj, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    return obj;
}

void idev_pfd_demo_create(lv_obj_t *screen, idev_pfd_demo_t *demo)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(idev_palette.background), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    demo->ann_ap = place(idev_annunciator_create(screen, "pfd.ann.ap"), 130, 14, 100, 36);
    demo->ann_stall = place(idev_annunciator_create(screen, "pfd.ann.stall"), 250, 14, 100, 36);
    demo->ann_bank = place(idev_annunciator_create(screen, "pfd.ann.bank"), 370, 14, 100, 36);
    demo->ann_overspeed =
        place(idev_annunciator_create(screen, "pfd.ann.overspeed"), 490, 14, 150, 36);

    demo->airspeed = place(idev_linear_tape_create(screen, "pfd.airspeed", false), 110, 70, 100, 390);
    demo->attitude = place(idev_attitude_create(screen, "pfd.roll", "pfd.pitch"), 222, 70, 356, 390);
    demo->altitude = place(idev_linear_tape_create(screen, "pfd.altitude", true), 590, 70, 110, 390);
}

void idev_pfd_demo_update(idev_pfd_demo_t *demo, float t_s, bool invalid)
{
    idev_pfd_frame_t f;
    idev_sim_pfd(t_s, &f);
    if (invalid) {
        f.pitch_deg = NAN;
        f.altitude_ft = NAN;
    }
    idev_attitude_set(demo->attitude, f.roll_deg, f.pitch_deg);
    idev_instrument_widget_set_value(demo->airspeed, f.airspeed_kt);
    idev_instrument_widget_set_value(demo->altitude, f.altitude_ft);

    const idev_instrument_t *ias = idev_widget_instrument(demo->airspeed, 0);
    idev_annunciator_set_level(demo->ann_stall, idev_alert_low(ias), false);
    idev_annunciator_set_level(demo->ann_overspeed, idev_alert_high(ias), false);
    idev_annunciator_set_level(demo->ann_bank, idev_widget_instrument(demo->attitude, 0)->alert,
                               false);
    idev_annunciator_set_level(demo->ann_ap, IDEV_LEVEL_NORMAL, f.autopilot);
}
