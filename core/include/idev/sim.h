/*
 * IDEV Instrument Kit - deterministic demo data generators.
 * The same function of time feeds the Qt and LVGL demos, so screenshots
 * taken at the same time stamp show the same values in both engines.
 * SPDX-License-Identifier: MIT
 */
#ifndef IDEV_SIM_H
#define IDEV_SIM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float speed_kmh;
    float rpm;
    float coolant_c;
    float fuel_pct;
    float oil_bar;
    bool brake;
} idev_car_frame_t;

typedef struct {
    float roll_deg;
    float pitch_deg;
    float airspeed_kt;
    float altitude_ft;
    bool autopilot;
} idev_pfd_frame_t;

/* Scenario period in seconds; both generators loop. */
#define IDEV_SIM_CAR_PERIOD_S 40.0f
#define IDEV_SIM_PFD_PERIOD_S 40.0f

void idev_sim_car(float t_s, idev_car_frame_t *out);
void idev_sim_pfd(float t_s, idev_pfd_frame_t *out);

#ifdef __cplusplus
}
#endif

#endif /* IDEV_SIM_H */
