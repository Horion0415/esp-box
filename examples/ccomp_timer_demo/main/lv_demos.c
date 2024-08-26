/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "bsp/esp-bsp.h"

#include "demos/lv_demos.h"

#include "ccomp_timer.h"
#include "esp_timer.h"

typedef struct {
    int64_t start;
    int64_t acc;
    char str1[15];
    char str2[15];
} PerfCounter;
static PerfCounter perf_counters[1] = {0};

static void perfmon_start(int ctr, const char *fmt1, const char *fmt2, ...)
{
    va_list args;
    va_start(args, fmt2);
    vsnprintf(perf_counters[ctr].str1, sizeof(perf_counters[ctr].str1), fmt1, args);
    vsnprintf(perf_counters[ctr].str2, sizeof(perf_counters[ctr].str2), fmt2, args);
    va_end(args);

    perf_counters[ctr].start = esp_timer_get_time();
}

static void perfmon_end(int ctr, int count)
{
    int64_t time_diff = esp_timer_get_time() - perf_counters[ctr].start;
    float time_in_sec = (float)time_diff / 1000000;
    float frequency = count / time_in_sec;

    printf("Perf ctr[%d], [%15s][%15s]: %.2f FPS (%.2f ms per operation)\n",
           ctr, perf_counters[ctr].str1, perf_counters[ctr].str2, frequency, time_in_sec * 1000 / count);
}

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();
    /* Initialize display and LVGL */
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /**
     * @brief Demos provided by LVGL.
     *
     * @note Only enable one demo every time.
     *
     */
    ccomp_timer_start();
    bsp_display_lock(0);

    lv_demo_widgets();      /* A widgets example. This is what you get out of the box */

    bsp_display_unlock();

    int64_t oper_time = ccomp_timer_stop();
    printf("time passed: %lld us\n", oper_time);

    static int count = 0;
    if (count % 10 == 0) {
        perfmon_start(0, "PFS", "camera");
    } else if (count % 10 == 9) {
        perfmon_end(0, 10);
    }
    count++;
}
