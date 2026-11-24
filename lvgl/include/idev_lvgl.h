/*
 * IDEV Instrument Kit - LVGL 9 widgets.
 * SPDX-License-Identifier: MIT
 *
 * Every widget is a plain lv_obj with a custom draw callback. Its data comes
 * from the shared configuration table (spec/instruments.json), looked up by
 * id - the same ids the Qt Quick module uses (Instrument.configId).
 * All functions must be called from the LVGL thread.
 */
#ifndef IDEV_LVGL_H
#define IDEV_LVGL_H

#include "idev/instrument.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Creation. Return NULL when the id is unknown or of the wrong kind. */
lv_obj_t *idev_round_gauge_create(lv_obj_t *parent, const char *config_id);
lv_obj_t *idev_linear_tape_create(lv_obj_t *parent, const char *config_id, bool mirrored);
lv_obj_t *idev_attitude_create(lv_obj_t *parent, const char *roll_id, const char *pitch_id);
lv_obj_t *idev_annunciator_create(lv_obj_t *parent, const char *config_id);

/* Data. NaN marks the value invalid ("---", needle hidden, ATT flag). */
void idev_instrument_widget_set_value(lv_obj_t *obj, float value); /* gauge, tape */
void idev_attitude_set(lv_obj_t *obj, float roll_deg, float pitch_deg);
void idev_annunciator_set_level(lv_obj_t *obj, idev_level_t level, bool active);
void idev_annunciator_acknowledge(lv_obj_t *obj);

/* State read-back (axis 0 = value or roll, axis 1 = pitch). */
const idev_instrument_t *idev_widget_instrument(lv_obj_t *obj, int axis);
bool idev_annunciator_is_lit(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /* IDEV_LVGL_H */
