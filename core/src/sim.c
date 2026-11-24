/* SPDX-License-Identifier: MIT */
#include "idev/sim.h"

#include <math.h>

#define IDEV_PI 3.14159265358979f

static float smoothstep01(float x)
{
    x = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    return x * x * (3.0f - 2.0f * x);
}

static float bump(float t, float centre, float width)
{
    const float d = (t - centre) / width;
    return expf(-d * d);
}

static float wrap(float t, float period)
{
    float r = fmodf(t, period);
    return r < 0.0f ? r + period : r;
}

void idev_sim_car(float t_s, idev_car_frame_t *out)
{
    static const float gear_top[] = {32.0f, 58.0f, 88.0f, 118.0f, 152.0f, 260.0f};
    const float t = wrap(t_s, IDEV_SIM_CAR_PERIOD_S);

    float speed;
    if (t < 4.0f) {
        speed = 0.0f;
    } else if (t < 26.0f) {
        speed = 150.0f * smoothstep01((t - 4.0f) / 22.0f);
    } else if (t < 34.0f) {
        speed = 150.0f - 28.0f * smoothstep01((t - 26.0f) / 8.0f);
    } else {
        speed = 122.0f * (1.0f - smoothstep01((t - 34.0f) / 6.0f));
    }

    float rpm = 850.0f;
    if (speed > 1.0f) {
        float lo = 0.0f;
        for (unsigned i = 0; i < sizeof gear_top / sizeof gear_top[0]; ++i) {
            if (speed <= gear_top[i] || i == sizeof gear_top / sizeof gear_top[0] - 1) {
                const float frac = (speed - lo) / (gear_top[i] - lo);
                const float top = i == 4 ? 6700.0f : 6300.0f;
                rpm = 1700.0f + (top - 1700.0f) * frac;
                break;
            }
            lo = gear_top[i];
        }
    }

    float coolant;
    if (t < 30.0f) {
        coolant = 70.0f + 48.0f * smoothstep01(t / 30.0f);
    } else {
        coolant = 118.0f - 48.0f * smoothstep01((t - 30.0f) / 10.0f);
    }

    out->speed_kmh = speed;
    out->rpm = rpm;
    out->coolant_c = coolant;
    out->fuel_pct = 26.0f - 14.0f * (t / IDEV_SIM_CAR_PERIOD_S);
    out->oil_bar = t < 1.5f ? 0.5f : 1.3f + rpm / 2500.0f;
    out->brake = t < 3.0f;
}

void idev_sim_pfd(float t_s, idev_pfd_frame_t *out)
{
    const float t = wrap(t_s, IDEV_SIM_PFD_PERIOD_S);
    out->roll_deg = 25.0f * sinf(2.0f * IDEV_PI * t / 16.0f) + 30.0f * bump(t, 20.0f, 2.5f);
    out->pitch_deg = 4.0f + 6.0f * sinf(2.0f * IDEV_PI * t / 13.0f);
    out->airspeed_kt =
        150.0f + 50.0f * sinf(2.0f * IDEV_PI * t / 40.0f) - 75.0f * bump(t, 34.0f, 2.5f);
    out->altitude_ft = 4500.0f + 1200.0f * sinf(2.0f * IDEV_PI * t / 40.0f + 0.5f);
    out->autopilot = t < 15.0f;
}
