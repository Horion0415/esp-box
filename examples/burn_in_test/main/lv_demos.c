/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "bsp/esp-bsp.h"

#include "demos/lv_demos.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600
#define BTN_SIZE 100

void create_checkerboard(void) {
    int rows = SCREEN_HEIGHT / BTN_SIZE;
    int cols = SCREEN_WIDTH / BTN_SIZE;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            lv_obj_t *btn = lv_btn_create(lv_scr_act());
            lv_obj_set_size(btn, BTN_SIZE, BTN_SIZE);
            lv_obj_align(btn, LV_ALIGN_TOP_LEFT, col * BTN_SIZE, row * BTN_SIZE);

            if ((row + col) % 2 == 0) {
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }
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
    bsp_display_lock(0);

    create_checkerboard();

    bsp_display_unlock();
}