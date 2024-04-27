/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "bsp/esp-bsp.h"

#include "demos/lv_demos.h"
#include "esp_log.h"
#define MEMORY_LEAK_DETECT          (0)
#if MEMORY_LEAK_DETECT
#define DETECT_THRESHOLD            (100)
#endif

#define TAG "main"

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

    size_t before_free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t before_free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    /**
     * @brief Demos provided by LVGL.
     *
     * @note Only enable one demo every time.
     *
     */
    bsp_display_lock(0);

    lv_demo_widgets();      /* A widgets example. This is what you get out of the box */

    bsp_display_unlock();

    size_t after_free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t after_free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    
    size_t internal_diff = before_free_internal - after_free_internal;
    size_t spiram_diff = before_free_spiram - after_free_spiram;
    ESP_LOGI(TAG, "Internal memory freed: %d", internal_diff);
    ESP_LOGI(TAG, "SPIRAM memory freed: %d", spiram_diff);
#if MEMORY_LEAK_DETECT
    if (internal_diff > DETECT_THRESHOLD || spiram_diff > DETECT_THRESHOLD) {
        ESP_LOGE(TAG, "Memory leak detected");
    }
#endif
}
