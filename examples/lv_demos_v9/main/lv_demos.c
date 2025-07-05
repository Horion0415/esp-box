/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "bsp/esp-bsp.h"

#include "demos/lv_demos.h"

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
            .buff_spiram = true,
        }
    };
    bsp_display_start_with_config(&cfg);

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    bsp_display_lock(0);

    //lv_demo_music();        /* A modern, smartphone-like music player demo. */
    lv_demo_render(LV_DEMO_RENDER_SCENE_BLEND_MODE, LV_OPA_COVER);   // 渲染带纹理的圆弧
    //lv_demo_vector_graphic_buffered();     // 或 lv_demo_vector_graphic_not_buffered();


    bsp_display_unlock();
}
