#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "bsp/esp-bsp.h"

#include "eye_animation.h"
//#include "eye_config.h"

static const char *TAG = "animated_eyes_main";

// LCD panel handles
static esp_lcd_panel_handle_t lcd_panel[1];
static esp_lcd_panel_io_handle_t lcd_io[1];

// Initialize LCD panel
static esp_err_t init_lcd_panel(int eye_index) {
    ESP_LOGI(TAG, "Initializing LCD panel #%d", eye_index);
    
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = (BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT) * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(bsp_display_new(&bsp_disp_cfg, &lcd_panel[eye_index], &lcd_io[eye_index]));
    esp_lcd_panel_disp_on_off(lcd_panel[eye_index], true);
    bsp_display_backlight_on();
    
    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting animated eyes demo");
    
    // Initialize LCD panels
    for (int i = 0; i < 1; i++) {
        init_lcd_panel(i);
    }
    
    // Initialize eye animation with our LCD panels
    eye_animation_init(lcd_panel, lcd_io);
    
    // Create eye animation task
    xTaskCreate(eye_animation_task, "eye_animation", 4096, NULL, 5, NULL);
    
    // Optional: Create eye control task (demonstrates how to control eye direction)
    // Comment this out if you want to implement your own control logic
    xTaskCreate(eye_control_task, "eye_control", 4096, NULL, 4, NULL);
    
    // Set eye to fixed position at top-left corner
    eye_set_fixed_position(0, 0);

    // After a period of time, switch to automatic movement mode
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    eye_set_auto_movement();

    // After another period, set a custom path
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    int16_t rectangle_path[4][2] = {
        {200, 200},  // Top-left
        {800, 200},  // Top-right
        {800, 800},  // Bottom-right
        {200, 800}   // Bottom-left
    };
    eye_set_custom_path(rectangle_path, 4, 2000); // Hold each position for 2 seconds
}