/*
 * IDEV Instrument Kit - shared instrument core (C99, no GUI dependency).
 * SPDX-License-Identifier: MIT
 *
 * This is the "business" layer shared by the LVGL widgets and the Qt Quick
 * module: configuration table, damping, alert evaluation with hysteresis,
 * zone lookup, value formatting and annunciator blink logic.
 */
#ifndef IDEV_INSTRUMENT_H
#define IDEV_INSTRUMENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IDEV_LEVEL_NORMAL = 0,
    IDEV_LEVEL_CAUTION = 1,
    IDEV_LEVEL_WARNING = 2
} idev_level_t;

typedef enum {
    IDEV_KIND_VALUE = 0,
    IDEV_KIND_ROUND_GAUGE,
    IDEV_KIND_LINEAR_TAPE,
    IDEV_KIND_ATTITUDE_AXIS,
    IDEV_KIND_ANNUNCIATOR
} idev_kind_t;

typedef struct {
    float from;
    float to;
    idev_level_t severity;
} idev_zone_t;

/* Thresholds are NAN when unused. */
typedef struct {
    float caution_low;
    float warning_low;
    float caution_high;
    float warning_high;
    float hysteresis;
} idev_alerts_t;

typedef struct {
    float start_angle;   /* degrees, clockwise from 12 o'clock */
    float sweep_angle;   /* degrees, clockwise */
    float major_step;    /* value units between labelled ticks */
    int minor_divisions; /* minor intervals per major interval */
    float label_scale;   /* tick labels show value * label_scale */
    const char *scale_caption;
} idev_gauge_params_t;

typedef struct {
    float visible_range; /* value units visible over the tape height */
    float major_step;
    int minor_divisions;
    float label_step;
} idev_tape_params_t;

typedef struct {
    float pitch_visible_range; /* degrees visible over the indicator height */
    float ladder_step;         /* degrees between pitch ladder rungs */
} idev_attitude_params_t;

typedef struct {
    float blink_hz;
} idev_annunciator_params_t;

typedef struct {
    const char *id;
    idev_kind_t kind;
    const char *label;
    const char *unit;
    float min;
    float max;
    int decimals;
    float damping_ms;
    const idev_zone_t *zones;
    size_t zone_count;
    idev_alerts_t alerts;
    idev_gauge_params_t gauge;
    idev_tape_params_t tape;
    idev_attitude_params_t attitude;
    idev_annunciator_params_t annunciator;
} idev_instrument_cfg_t;

/* Colours as 0xRRGGBB. */
typedef struct {
    uint32_t normal, caution, warning;
    uint32_t background, surface, surface_raised;
    uint32_t foreground, dim, needle;
    uint32_t sky, ground, symbol;
} idev_palette_t;

/* Generated from spec/instruments.json by tools/gen_instruments.py. */
extern const idev_instrument_cfg_t idev_cfg_table[];
extern const size_t idev_cfg_count;
extern const idev_palette_t idev_palette;

const idev_instrument_cfg_t *idev_cfg_find(const char *id);
uint32_t idev_level_color(idev_level_t level);

/* Runtime state of one instrument. Plain struct: no allocation. */
typedef struct {
    const idev_instrument_cfg_t *cfg;
    float target;  /* last value received (may be outside min/max) */
    float display; /* damped value, clamped to [min, max] */
    bool valid;    /* false until a finite value arrives, or after NaN */
    idev_level_t alert;
} idev_instrument_t;

void idev_instrument_init(idev_instrument_t *inst, const idev_instrument_cfg_t *cfg);
/* NaN marks the data as invalid (sensor lost). Alerts use the raw value. */
void idev_instrument_set_value(idev_instrument_t *inst, float value);
/* Advance damping by dt_ms. Returns true when the displayed value moved. */
bool idev_instrument_step(idev_instrument_t *inst, float dt_ms);
/* True while the displayed value has not reached the target. */
bool idev_instrument_settling(const idev_instrument_t *inst);

/*
 * Alert split by side of the scale, for annunciators such as STALL (low) and
 * OVERSPEED (high). The side is decided by the raw value relative to the
 * middle of [min, max].
 */
idev_level_t idev_alert_high(const idev_instrument_t *inst);
idev_level_t idev_alert_low(const idev_instrument_t *inst);

float idev_ratio(const idev_instrument_cfg_t *cfg, float value); /* 0..1, clamped */
idev_level_t idev_zone_at(const idev_instrument_cfg_t *cfg, float value);
idev_level_t idev_alert_eval(const idev_alerts_t *alerts, float value, idev_level_t previous);
/* Formats value with cfg->decimals; writes "---" when !valid. */
int idev_format(const idev_instrument_cfg_t *cfg, float value, bool valid, char *buf, size_t size);
/* Formats a scale label (value * gauge.label_scale, no decimals). */
int idev_format_scale_label(const idev_instrument_cfg_t *cfg, float value, char *buf, size_t size);

/*
 * Annunciator: NORMAL is dark unless `active` (advisory, steady);
 * CAUTION is lit steady; WARNING flashes at blink_hz until acknowledged,
 * then stays lit steady. Lit during the first half of each period.
 */
bool idev_annunciator_lit(idev_level_t level, bool active, bool acknowledged, uint32_t t_ms,
                          float blink_hz);

#ifdef __cplusplus
}
#endif

#endif /* IDEV_INSTRUMENT_H */
