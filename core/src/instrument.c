/* SPDX-License-Identifier: MIT */
#include "idev/instrument.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

const idev_instrument_cfg_t *idev_cfg_find(const char *id)
{
    if (id == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < idev_cfg_count; ++i) {
        if (strcmp(idev_cfg_table[i].id, id) == 0) {
            return &idev_cfg_table[i];
        }
    }
    return NULL;
}

uint32_t idev_level_color(idev_level_t level)
{
    switch (level) {
    case IDEV_LEVEL_WARNING:
        return idev_palette.warning;
    case IDEV_LEVEL_CAUTION:
        return idev_palette.caution;
    case IDEV_LEVEL_NORMAL:
    default:
        return idev_palette.normal;
    }
}

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

void idev_instrument_init(idev_instrument_t *inst, const idev_instrument_cfg_t *cfg)
{
    memset(inst, 0, sizeof(*inst));
    inst->cfg = cfg;
    inst->valid = false;
    inst->alert = IDEV_LEVEL_NORMAL;
    if (cfg != NULL) {
        inst->target = cfg->min;
        inst->display = cfg->min;
    }
}

void idev_instrument_set_value(idev_instrument_t *inst, float value)
{
    if (inst->cfg == NULL) {
        return;
    }
    if (!isfinite(value)) {
        inst->valid = false;
        inst->alert = IDEV_LEVEL_NORMAL;
        return;
    }
    if (!inst->valid) {
        /* First valid sample after start or loss: no sweep from a stale value. */
        inst->display = clampf(value, inst->cfg->min, inst->cfg->max);
    }
    inst->valid = true;
    inst->target = value;
    inst->alert = idev_alert_eval(&inst->cfg->alerts, value, inst->alert);
}

static float settle_epsilon(const idev_instrument_cfg_t *cfg)
{
    return (cfg->max - cfg->min) * 1e-4f;
}

bool idev_instrument_settling(const idev_instrument_t *inst)
{
    if (inst->cfg == NULL || !inst->valid) {
        return false;
    }
    const float goal = clampf(inst->target, inst->cfg->min, inst->cfg->max);
    return fabsf(goal - inst->display) > settle_epsilon(inst->cfg);
}

bool idev_instrument_step(idev_instrument_t *inst, float dt_ms)
{
    if (inst->cfg == NULL || !inst->valid || dt_ms <= 0.0f) {
        return false;
    }
    const float goal = clampf(inst->target, inst->cfg->min, inst->cfg->max);
    const float before = inst->display;
    const float tau = inst->cfg->damping_ms;
    if (tau <= 0.0f) {
        inst->display = goal;
    } else {
        const float alpha = 1.0f - expf(-dt_ms / tau);
        inst->display += (goal - inst->display) * alpha;
        if (fabsf(goal - inst->display) <= settle_epsilon(inst->cfg)) {
            inst->display = goal;
        }
    }
    return inst->display != before;
}

static bool on_high_side(const idev_instrument_t *inst)
{
    return inst->target >= 0.5f * (inst->cfg->min + inst->cfg->max);
}

idev_level_t idev_alert_high(const idev_instrument_t *inst)
{
    if (inst->cfg == NULL || !inst->valid) {
        return IDEV_LEVEL_NORMAL;
    }
    return on_high_side(inst) ? inst->alert : IDEV_LEVEL_NORMAL;
}

idev_level_t idev_alert_low(const idev_instrument_t *inst)
{
    if (inst->cfg == NULL || !inst->valid) {
        return IDEV_LEVEL_NORMAL;
    }
    return on_high_side(inst) ? IDEV_LEVEL_NORMAL : inst->alert;
}

float idev_ratio(const idev_instrument_cfg_t *cfg, float value)
{
    const float span = cfg->max - cfg->min;
    if (span <= 0.0f || !isfinite(value)) {
        return 0.0f;
    }
    return clampf((value - cfg->min) / span, 0.0f, 1.0f);
}

idev_level_t idev_zone_at(const idev_instrument_cfg_t *cfg, float value)
{
    for (size_t i = 0; i < cfg->zone_count; ++i) {
        const idev_zone_t *z = &cfg->zones[i];
        if (value >= z->from && value <= z->to) {
            return z->severity;
        }
    }
    return IDEV_LEVEL_NORMAL;
}

static idev_level_t raw_level(const idev_alerts_t *a, float v, float margin)
{
    /* margin > 0 relaxes thresholds toward the safe side (used to leave a level). */
    if ((!isnan(a->warning_high) && v >= a->warning_high - margin) ||
        (!isnan(a->warning_low) && v <= a->warning_low + margin)) {
        return IDEV_LEVEL_WARNING;
    }
    if ((!isnan(a->caution_high) && v >= a->caution_high - margin) ||
        (!isnan(a->caution_low) && v <= a->caution_low + margin)) {
        return IDEV_LEVEL_CAUTION;
    }
    return IDEV_LEVEL_NORMAL;
}

idev_level_t idev_alert_eval(const idev_alerts_t *alerts, float value, idev_level_t previous)
{
    if (!isfinite(value)) {
        return IDEV_LEVEL_NORMAL;
    }
    const idev_level_t now = raw_level(alerts, value, 0.0f);
    if (now >= previous) {
        return now; /* escalate immediately */
    }
    const float h = isnan(alerts->hysteresis) ? 0.0f : alerts->hysteresis;
    const idev_level_t held = raw_level(alerts, value, h);
    return held > previous ? previous : (held > now ? held : now);
}

int idev_format(const idev_instrument_cfg_t *cfg, float value, bool valid, char *buf, size_t size)
{
    if (!valid || !isfinite(value)) {
        return snprintf(buf, size, "---");
    }
    const int decimals = cfg->decimals < 0 ? 0 : (cfg->decimals > 6 ? 6 : cfg->decimals);
    /* Avoid printing "-0". */
    const float p = powf(10.0f, (float)decimals);
    float rounded = roundf(value * p) / p;
    if (rounded == 0.0f) {
        rounded = 0.0f;
    }
    return snprintf(buf, size, "%.*f", decimals, (double)rounded);
}

int idev_format_scale_label(const idev_instrument_cfg_t *cfg, float value, char *buf, size_t size)
{
    const float scale = cfg->gauge.label_scale > 0.0f ? cfg->gauge.label_scale : 1.0f;
    return snprintf(buf, size, "%.0f", (double)roundf(value * scale));
}

bool idev_annunciator_lit(idev_level_t level, bool active, bool acknowledged, uint32_t t_ms,
                          float blink_hz)
{
    switch (level) {
    case IDEV_LEVEL_WARNING:
        if (acknowledged || blink_hz <= 0.0f) {
            return true;
        } else {
            const uint32_t period = (uint32_t)(1000.0f / blink_hz);
            return period == 0 ? true : (t_ms % period) < period / 2;
        }
    case IDEV_LEVEL_CAUTION:
        return true;
    case IDEV_LEVEL_NORMAL:
    default:
        return active;
    }
}
