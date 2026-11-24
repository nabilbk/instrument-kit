/* SPDX-License-Identifier: MIT
 * Headless LVGL host: renders a demo into a memory frame buffer and writes a PNG.
 * Also measures render time on the host CPU and LVGL heap use per widget.
 *
 *   idev_lvgl_headless --demo car|pfd [--time 26] [--png out.png] [--invalid]
 *   idev_lvgl_headless --demo car --bench 300
 *   idev_lvgl_headless --memreport
 *   idev_lvgl_headless --selftest
 */
#include "idev_demos.h"

#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t g_ms;
static uint32_t tick_cb(void)
{
    return g_ms;
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    (void)area;
    (void)px;
    lv_display_flush_ready(disp); /* DIRECT mode: the buffer is the frame */
}

static int write_png(const char *path, const uint8_t *argb, int w, int h, int stride)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        perror(path);
        return -1;
    }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if (png == NULL || info == NULL || setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return -1;
    }
    png_init_io(png, fp);
    png_set_IHDR(png, info, (png_uint_32)w, (png_uint_32)h, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    uint8_t *row = malloc((size_t)w * 3);
    for (int y = 0; y < h; ++y) {
        const uint8_t *src = argb + (size_t)y * (size_t)stride;
        for (int x = 0; x < w; ++x) { /* LVGL 32-bit: B, G, R, A in memory */
            row[x * 3 + 0] = src[x * 4 + 2];
            row[x * 3 + 1] = src[x * 4 + 1];
            row[x * 3 + 2] = src[x * 4 + 0];
        }
        png_write_row(png, row);
    }
    free(row);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return 0;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

typedef struct {
    int is_pfd;
    idev_car_demo_t car;
    idev_pfd_demo_t pfd;
} demo_t;

static void demo_update(demo_t *d, float t, bool invalid)
{
    if (d->is_pfd) {
        idev_pfd_demo_update(&d->pfd, t, invalid);
    } else {
        idev_car_demo_update(&d->car, t, invalid);
    }
}

static size_t heap_used(void)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    return m.total_size - m.free_size;
}

static void memreport(void)
{
    lv_obj_t *scr = lv_screen_active();
    printf("LVGL heap use per widget (LV_MEM builtin allocator, %u-bit pointers, incl. lv_obj, "
           "allocator headers, timer):\n",
           (unsigned)(sizeof(void *) * 8));
    for (int i = 0; i < 4; ++i) {
        const size_t before = heap_used();
        lv_obj_t *o = NULL;
        const char *name = "";
        switch (i) {
        case 0: o = idev_round_gauge_create(scr, "car.rpm"); name = "RoundGauge"; break;
        case 1: o = idev_linear_tape_create(scr, "pfd.airspeed", false); name = "LinearTape"; break;
        case 2: o = idev_attitude_create(scr, "pfd.roll", "pfd.pitch"); name = "AttitudeIndicator"; break;
        default: o = idev_annunciator_create(scr, "car.ann.oil"); name = "Annunciator"; break;
        }
        const size_t after = heap_used();
        printf("  %-18s %5zu bytes\n", name, after - before);
        lv_obj_delete(o);
    }
}

static int g_fail;
#define EXPECT(cond)                                                                    \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            ++g_fail;                                                                   \
            fprintf(stderr, "selftest: %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                               \
    } while (0)

static void run_for(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 10) {
        g_ms += 10;
        lv_timer_handler();
    }
}

/* Widget-level checks through the public LVGL API (S1.3, S3.3, S7, S8). */
static int selftest(void)
{
    lv_obj_t *scr = lv_screen_active();
    EXPECT(idev_round_gauge_create(scr, "no.such.id") == NULL);
    EXPECT(idev_round_gauge_create(scr, "pfd.airspeed") == NULL); /* wrong kind */
    EXPECT(idev_attitude_create(scr, "pfd.roll", "car.rpm") == NULL);

    lv_obj_t *g = idev_round_gauge_create(scr, "car.rpm");
    EXPECT(g != NULL);
    EXPECT(!idev_widget_instrument(g, 0)->valid);
    idev_instrument_widget_set_value(g, 1000.0f);
    idev_instrument_widget_set_value(g, 5000.0f);
    run_for(1000);
    EXPECT(fabsf(idev_widget_instrument(g, 0)->display - 5000.0f) < 1.0f);
    EXPECT(!idev_instrument_settling(idev_widget_instrument(g, 0)));
    idev_instrument_widget_set_value(g, 6600.0f);
    EXPECT(idev_widget_instrument(g, 0)->alert == IDEV_LEVEL_WARNING);
    idev_instrument_widget_set_value(g, NAN);
    EXPECT(!idev_widget_instrument(g, 0)->valid);
    EXPECT(idev_widget_instrument(g, 0)->alert == IDEV_LEVEL_NORMAL);

    lv_obj_t *a = idev_annunciator_create(scr, "car.ann.temp");
    EXPECT(a != NULL);
    g_ms = 10000; /* period boundary for a 2 Hz flash */
    idev_annunciator_set_level(a, IDEV_LEVEL_NORMAL, false);
    EXPECT(!idev_annunciator_is_lit(a));
    idev_annunciator_set_level(a, IDEV_LEVEL_NORMAL, true);
    EXPECT(idev_annunciator_is_lit(a));
    idev_annunciator_set_level(a, IDEV_LEVEL_CAUTION, false);
    EXPECT(idev_annunciator_is_lit(a));
    idev_annunciator_set_level(a, IDEV_LEVEL_WARNING, false);
    EXPECT(idev_annunciator_is_lit(a));
    run_for(300); /* 10300 ms: second half of the period */
    EXPECT(!idev_annunciator_is_lit(a));
    run_for(250); /* 10550 ms: lit again */
    EXPECT(idev_annunciator_is_lit(a));
    run_for(240); /* 10790 ms: dark (timer granularity is 50 ms) */
    EXPECT(!idev_annunciator_is_lit(a));
    lv_obj_send_event(a, LV_EVENT_CLICKED, NULL); /* acknowledge by click */
    EXPECT(idev_annunciator_is_lit(a));
    run_for(300);
    EXPECT(idev_annunciator_is_lit(a)); /* steady once acknowledged */
    idev_annunciator_set_level(a, IDEV_LEVEL_CAUTION, false);
    idev_annunciator_set_level(a, IDEV_LEVEL_WARNING, false); /* re-armed */
    g_ms = 20300;
    lv_timer_handler();
    EXPECT(!idev_annunciator_is_lit(a));

    lv_obj_delete(g);
    lv_obj_delete(a);
    printf("selftest: %s\n", g_fail == 0 ? "all checks passed" : "FAILED");
    return g_fail == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    const char *demo_name = "car";
    const char *png_path = NULL;
    float t = 26.0f;
    int bench = 0;
    bool invalid = false;
    bool mem = false;
    bool self = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--demo") && i + 1 < argc) demo_name = argv[++i];
        else if (!strcmp(argv[i], "--time") && i + 1 < argc) t = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--png") && i + 1 < argc) png_path = argv[++i];
        else if (!strcmp(argv[i], "--bench") && i + 1 < argc) bench = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--invalid")) invalid = true;
        else if (!strcmp(argv[i], "--memreport")) mem = true;
        else if (!strcmp(argv[i], "--selftest")) self = true;
        else {
            fprintf(stderr, "usage: %s --demo car|pfd [--time s] [--png file] [--invalid] "
                            "[--bench frames] | --memreport | --selftest\n", argv[0]);
            return 2;
        }
    }

    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *disp = lv_display_create(IDEV_DEMO_W, IDEV_DEMO_H);
    const uint32_t stride = lv_draw_buf_width_to_stride(IDEV_DEMO_W, lv_display_get_color_format(disp));
    const uint32_t size = stride * IDEV_DEMO_H;
    uint8_t *fb = aligned_alloc(64, (size + 63) / 64 * 64);
    if (fb == NULL) {
        return 1;
    }
    memset(fb, 0, size);
    lv_display_set_buffers(disp, fb, NULL, size, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    if (mem) {
        memreport();
        return 0;
    }
    if (self) {
        return selftest();
    }

    demo_t d;
    memset(&d, 0, sizeof d);
    d.is_pfd = !strcmp(demo_name, "pfd");
    if (d.is_pfd) {
        idev_pfd_demo_create(lv_screen_active(), &d.pfd);
    } else {
        idev_car_demo_create(lv_screen_active(), &d.car);
    }

    /* Run the scenario for 2 s of virtual time up to t, 60 Hz steps, so damping settles
     * exactly as on a device. 125 steps x 16 ms = 2000 ms: blink phase is "lit". */
    for (int step = 0; step < 125; ++step) {
        const float ts = t - 2.0f + (float)step * 0.016f;
        demo_update(&d, ts, invalid);
        g_ms += 16;
        lv_timer_handler();
    }
    demo_update(&d, t, invalid);
    lv_timer_handler();
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(disp);

    if (png_path != NULL && write_png(png_path, fb, IDEV_DEMO_W, IDEV_DEMO_H, (int)stride) != 0) {
        return 1;
    }
    if (png_path != NULL) {
        printf("wrote %s (%s, t=%.2f s%s)\n", png_path, demo_name, (double)t,
               invalid ? ", invalid data" : "");
    }

    if (bench > 0) {
        /* Typical frame: new values every frame, only dirty areas redrawn. */
        double worst = 0.0, total = 0.0;
        for (int i = 0; i < bench; ++i) {
            demo_update(&d, t + (float)i * 0.016f, false);
            g_ms += 16;
            const double t0 = now_ms();
            lv_timer_handler();
            lv_refr_now(disp);
            const double dt = now_ms() - t0;
            total += dt;
            worst = dt > worst ? dt : worst;
        }
        /* Full-screen redraw, worst case. */
        double full = 0.0;
        for (int i = 0; i < bench; ++i) {
            lv_obj_invalidate(lv_screen_active());
            const double t0 = now_ms();
            lv_refr_now(disp);
            full += now_ms() - t0;
        }
        printf("bench %s %dx%d, %d frames on host CPU: updated frame avg %.2f ms (max %.2f ms), "
               "full redraw avg %.2f ms\n",
               demo_name, IDEV_DEMO_W, IDEV_DEMO_H, bench, total / bench, worst, full / bench);
    }
    return 0;
}
