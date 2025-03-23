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

static const char *TAG = "animated_eyes_main";

#define NUM_EYES 1

#define LH_WINK_PIN -1 // Left wink pin (set to -1 for no pin)
#define RH_WINK_PIN -1 // Right wink pin (set to -1 for no pin)

#define EYE_1_CS    5                 // TFT 1 chip select pin
#define EYE_2_CS   -1                 // TFT 2 chip select pin
#define EYE_1_ROT  1                 // TFT 1 rotation
#define EYE_2_ROT  1                 // TFT 2 rotation
#define EYE_1_XPOSITION  0         // x offset of eye 1 image on display
#define EYE_2_XPOSITION  320 - 128 // x offset of eye 2 image on display

// LCD panel handles
static esp_lcd_panel_handle_t lcd_panel[NUM_EYES];
static esp_lcd_panel_io_handle_t lcd_io[NUM_EYES];

// Initialize LCD panel
static esp_err_t init_lcd_panel(int eye_index) {
    ESP_LOGI(TAG, "Initializing LCD panel #%d", eye_index);
    
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = (BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT) * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(bsp_display_new(&bsp_disp_cfg, &lcd_panel[eye_index], &lcd_io[eye_index]));
    esp_lcd_panel_disp_on_off(lcd_panel[eye_index], true);
    
    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting animated eyes demo");
    
    // Initialize LCD panels
    for (int i = 0; i < NUM_EYES; i++) {
        init_lcd_panel(i);
    }
    bsp_display_backlight_on();
    
#if (NUM_EYES == 2)
  eyeInfo_t eyeInfo[] = {
    { LH_WINK_PIN, EYE_1_ROT, EYE_1_XPOSITION }, // Left eye chip select and wink pin, rotation and offset
    { RH_WINK_PIN, EYE_2_ROT, EYE_2_XPOSITION }, // Right eye chip select and wink pin, rotation and offset
  };
#else
  eyeInfo_t eyeInfo[] = {
    { LH_WINK_PIN, EYE_1_ROT, EYE_1_XPOSITION }, // Eye chip select and wink pin, rotation and offset
  };
#endif

    // Initialize eye animation with our LCD panels
    eye_animation_init(lcd_panel, eyeInfo, NUM_EYES);
    
    eye_animation_start();
    
    // Set eye to fixed position at top-left corner
    eye_set_fixed_position(0, 0);

    // After a period of time, switch to automatic movement mode
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    eye_set_auto_movement();

    // After another period, set a custom path
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    int16_t rectangle_path[4][2] = {
        {200, 200},  // Top-left
        {800, 200},  // Top-right
        {800, 800},  // Bottom-right
        {200, 800}   // Bottom-left
    };
    eye_set_custom_path(rectangle_path, 4, 2000); // Hold each position for 2 seconds
}