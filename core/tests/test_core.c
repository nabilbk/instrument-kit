/* SPDX-License-Identifier: MIT
 * Unit tests for idev_core. Plain C, no framework: exits non-zero on failure. */
#include "idev/instrument.h"
#include "idev/sim.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;
static int checks;

#define CHECK(cond)                                                                     \
    do {                                                                                \
        ++checks;                                                                       \
        if (!(cond)) {                                                                  \
            ++failures;                                                                 \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                               \
    } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(fabsf((float)(a) - (float)(b)) <= (eps))

static void test_lookup(void)
{
    CHECK(idev_cfg_find("car.rpm") != NULL);
    CHECK(idev_cfg_find("pfd.airspeed")->kind == IDEV_KIND_LINEAR_TAPE);
    CHECK(idev_cfg_find("does.not.exist") == NULL);
    CHECK(idev_cfg_find(NULL) == NULL);
    CHECK(idev_palette.warning == 0xE8443Au);
}

static void test_damping(void)
{
    const idev_instrument_cfg_t *cfg = idev_cfg_find("car.rpm"); /* tau = 80 ms */
    idev_instrument_t in;
    idev_instrument_init(&in, cfg);
    CHECK(!in.valid);
    idev_instrument_set_value(&in, 1000.0f);
    CHECK(in.valid);
    CHECK_NEAR(in.display, 1000.0f, 0.01f); /* first sample: no sweep */
    idev_instrument_set_value(&in, 5000.0f);
    CHECK(idev_instrument_settling(&in));
    idev_instrument_step(&in, 80.0f); /* one time constant: 63.2 % */
    CHECK_NEAR(in.display, 1000.0f + 4000.0f * 0.632f, 5.0f);
    for (int i = 0; i < 100; ++i) {
        idev_instrument_step(&in, 16.0f);
    }
    CHECK_NEAR(in.display, 5000.0f, 0.001f);
    CHECK(!idev_instrument_settling(&in));
    /* Display is clamped to the scale, target keeps the raw value. */
    idev_instrument_set_value(&in, 9000.0f);
    for (int i = 0; i < 100; ++i) {
        idev_instrument_step(&in, 16.0f);
    }
    CHECK_NEAR(in.display, 8000.0f, 0.001f);
    CHECK_NEAR(in.target, 9000.0f, 0.001f);
}

static void test_invalid(void)
{
    idev_instrument_t in;
    char buf[16];
    idev_instrument_init(&in, idev_cfg_find("car.coolant"));
    idev_instrument_set_value(&in, 120.0f);
    CHECK(in.alert == IDEV_LEVEL_WARNING);
    idev_instrument_set_value(&in, NAN);
    CHECK(!in.valid);
    CHECK(in.alert == IDEV_LEVEL_NORMAL);
    idev_format(in.cfg, in.display, in.valid, buf, sizeof buf);
    CHECK(strcmp(buf, "---") == 0);
    CHECK(!idev_instrument_step(&in, 16.0f));
}

static void test_alerts_hysteresis(void)
{
    const idev_alerts_t *a = &idev_cfg_find("car.rpm")->alerts; /* 6000 / 6500, h = 100 */
    idev_level_t l = IDEV_LEVEL_NORMAL;
    l = idev_alert_eval(a, 5999.0f, l);
    CHECK(l == IDEV_LEVEL_NORMAL);
    l = idev_alert_eval(a, 6000.0f, l);
    CHECK(l == IDEV_LEVEL_CAUTION);
    l = idev_alert_eval(a, 6600.0f, l);
    CHECK(l == IDEV_LEVEL_WARNING);
    l = idev_alert_eval(a, 6450.0f, l); /* inside hysteresis band: hold */
    CHECK(l == IDEV_LEVEL_WARNING);
    l = idev_alert_eval(a, 6390.0f, l); /* left band: caution */
    CHECK(l == IDEV_LEVEL_CAUTION);
    l = idev_alert_eval(a, 5950.0f, l);
    CHECK(l == IDEV_LEVEL_CAUTION);
    l = idev_alert_eval(a, 5890.0f, l);
    CHECK(l == IDEV_LEVEL_NORMAL);
    l = idev_alert_eval(a, 7000.0f, l); /* jump straight to warning */
    CHECK(l == IDEV_LEVEL_WARNING);

    const idev_alerts_t *f = &idev_cfg_find("car.fuel")->alerts; /* low side 20 / 10 */
    CHECK(idev_alert_eval(f, 50.0f, IDEV_LEVEL_NORMAL) == IDEV_LEVEL_NORMAL);
    CHECK(idev_alert_eval(f, 15.0f, IDEV_LEVEL_NORMAL) == IDEV_LEVEL_CAUTION);
    CHECK(idev_alert_eval(f, 9.0f, IDEV_LEVEL_CAUTION) == IDEV_LEVEL_WARNING);
    CHECK(idev_alert_eval(f, 10.5f, IDEV_LEVEL_WARNING) == IDEV_LEVEL_WARNING);
    CHECK(idev_alert_eval(f, 11.5f, IDEV_LEVEL_WARNING) == IDEV_LEVEL_CAUTION);

    idev_instrument_t ias;
    idev_instrument_init(&ias, idev_cfg_find("pfd.airspeed"));
    idev_instrument_set_value(&ias, 50.0f);
    CHECK(idev_alert_low(&ias) == IDEV_LEVEL_WARNING);
    CHECK(idev_alert_high(&ias) == IDEV_LEVEL_NORMAL);
    idev_instrument_set_value(&ias, 290.0f);
    CHECK(idev_alert_low(&ias) == IDEV_LEVEL_NORMAL);
    CHECK(idev_alert_high(&ias) == IDEV_LEVEL_WARNING);

    const idev_alerts_t *alt =&idev_cfg_find("pfd.altitude")->alerts; /* none */
    CHECK(idev_alert_eval(alt, 1e9f, IDEV_LEVEL_NORMAL) == IDEV_LEVEL_NORMAL);
}

static void test_zones_ratio_format(void)
{
    const idev_instrument_cfg_t *rpm = idev_cfg_find("car.rpm");
    char buf[16];
    CHECK(idev_zone_at(rpm, 3000.0f) == IDEV_LEVEL_NORMAL);
    CHECK(idev_zone_at(rpm, 6200.0f) == IDEV_LEVEL_CAUTION);
    CHECK(idev_zone_at(rpm, 7900.0f) == IDEV_LEVEL_WARNING);
    CHECK_NEAR(idev_ratio(rpm, 4000.0f), 0.5f, 1e-6f);
    CHECK_NEAR(idev_ratio(rpm, -10.0f), 0.0f, 1e-6f);
    CHECK_NEAR(idev_ratio(rpm, 1e6f), 1.0f, 1e-6f);
    idev_format_scale_label(rpm, 7000.0f, buf, sizeof buf);
    CHECK(strcmp(buf, "7") == 0);
    idev_format(rpm, 6234.6f, true, buf, sizeof buf);
    CHECK(strcmp(buf, "6235") == 0);
    idev_format(idev_cfg_find("car.oilPressure"), 2.349f, true, buf, sizeof buf);
    CHECK(strcmp(buf, "2.3") == 0);
    idev_format(idev_cfg_find("pfd.roll"), -0.2f, true, buf, sizeof buf);
    CHECK(strcmp(buf, "0") == 0);
}

static void test_annunciator(void)
{
    CHECK(!idev_annunciator_lit(IDEV_LEVEL_NORMAL, false, false, 0, 2.0f));
    CHECK(idev_annunciator_lit(IDEV_LEVEL_NORMAL, true, false, 0, 2.0f));
    CHECK(idev_annunciator_lit(IDEV_LEVEL_CAUTION, false, false, 260, 2.0f));
    CHECK(idev_annunciator_lit(IDEV_LEVEL_WARNING, false, false, 0, 2.0f));
    CHECK(idev_annunciator_lit(IDEV_LEVEL_WARNING, false, false, 249, 2.0f));
    CHECK(!idev_annunciator_lit(IDEV_LEVEL_WARNING, false, false, 250, 2.0f));
    CHECK(idev_annunciator_lit(IDEV_LEVEL_WARNING, false, false, 500, 2.0f));
    CHECK(idev_annunciator_lit(IDEV_LEVEL_WARNING, false, true, 250, 2.0f)); /* acked: steady */
}

static void test_sim_deterministic(void)
{
    idev_car_frame_t a, b;
    idev_sim_car(12.5f, &a);
    idev_sim_car(12.5f + IDEV_SIM_CAR_PERIOD_S, &b);
    CHECK_NEAR(a.speed_kmh, b.speed_kmh, 1e-3f);
    CHECK_NEAR(a.rpm, b.rpm, 1e-2f);
    for (float t = 0.0f; t < IDEV_SIM_CAR_PERIOD_S; t += 0.25f) {
        idev_sim_car(t, &a);
        CHECK(a.rpm >= 0.0f && a.rpm <= 8000.0f);
        CHECK(a.speed_kmh >= 0.0f && a.speed_kmh <= 260.0f);
    }
    idev_pfd_frame_t p;
    for (float t = 0.0f; t < IDEV_SIM_PFD_PERIOD_S; t += 0.25f) {
        idev_sim_pfd(t, &p);
        CHECK(p.pitch_deg > -90.0f && p.pitch_deg < 90.0f);
        CHECK(p.airspeed_kt > 0.0f);
    }
}

int main(void)
{
    test_lookup();
    test_damping();
    test_invalid();
    test_alerts_hysteresis();
    test_zones_ratio_format();
    test_annunciator();
    test_sim_deterministic();
    printf("idev_core_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
