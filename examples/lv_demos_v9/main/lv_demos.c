/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "bsp/esp-bsp.h"

#include "demos/lv_demos.h"

void ui_setting_screen_init(void)
{
    lv_obj_t * ui_setting;
    lv_obj_t * ui_PanelSetmain;
    lv_obj_t * ui_ListSetting;
    lv_obj_t * ui_ListItem1;
    lv_obj_t * ui_ListItem2;
    lv_obj_t * ui_ListItem3;
    lv_obj_t * ui_ListItem4;
    lv_obj_t * ui_ListItem5;
    lv_obj_t * ui_ListItem6;
    lv_obj_t * ui_ListItem7;
    lv_obj_t * ui_ListItem8;
    lv_obj_t * ui_ListItem9;
    lv_obj_t * ui_ListItem10;
    lv_obj_t * ui_ListItem11;
    lv_obj_t * ui_ListItem12;
    lv_obj_t * ui_ListItem13;
    lv_obj_t * ui_ListItem14;
    lv_obj_t * ui_ListItem15;
    lv_obj_t * ui_ListItem16;
    lv_obj_t * ui_ListItem17;
    lv_obj_t * ui_ListItem18;

    ui_setting = lv_obj_create(lv_scr_act());
    lv_obj_remove_flag(ui_setting, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    lv_obj_set_style_bg_opa(ui_setting, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_setting, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    ESP_LOGW("UI", "ui_setting: %p", ui_setting);
    lv_obj_set_size(ui_setting, 320, 240);

#if 1
    ui_PanelSetmain = lv_obj_create(ui_setting);
    lv_obj_set_width(ui_PanelSetmain, 320);
    lv_obj_set_height(ui_PanelSetmain, 240);
    lv_obj_set_align(ui_PanelSetmain, LV_ALIGN_CENTER);
    lv_obj_remove_flag(ui_PanelSetmain, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_border_width(ui_PanelSetmain, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    ESP_LOGW("UI", "ui_PanelSetmain: %p", ui_PanelSetmain);
#else
    ui_PanelSetmain = ui_setting;
#endif

    // 创建列表容器
    ui_ListSetting = lv_list_create(ui_PanelSetmain);
    lv_obj_set_width(ui_ListSetting, 320);
    lv_obj_set_height(ui_ListSetting, 240);
    lv_obj_set_x(ui_ListSetting, 0);
    lv_obj_set_y(ui_ListSetting, 50);
    lv_obj_set_align(ui_ListSetting, LV_ALIGN_CENTER);
    // lv_obj_remove_flag(ui_ListSetting, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    
    // 设置列表样式
    lv_obj_set_style_bg_color(ui_ListSetting, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ListSetting, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_ListSetting, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_ListSetting, lv_color_hex(0xCCCCCC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_ListSetting, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ESP_LOGW("UI", "ui_ListSetting: %p", ui_ListSetting);
    
    // 添加列表项
    ui_ListItem1 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_SETTINGS, "System Settings");
    // lv_obj_set_style_text_font(ui_ListItem1, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem1, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    ESP_LOGW("UI", "ui_ListItem1: %p", ui_ListItem1);
    
    ui_ListItem2 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_WIFI, "Network Settings");
    // lv_obj_set_style_text_font(ui_ListItem2, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem2, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem3 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_WIFI, "Display Settings");
    // lv_obj_set_style_text_font(ui_ListItem3, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem3, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem4 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_VOLUME_MAX, "Audio Settings");
    // lv_obj_set_style_text_font(ui_ListItem4, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem4, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem5 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_SHUFFLE, "Time Settings");
    // lv_obj_set_style_text_font(ui_ListItem5, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem5, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem6 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_DOWNLOAD, "Software Update");
    // lv_obj_set_style_text_font(ui_ListItem6, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem6, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem7 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_DOWNLOAD, "About System");
    // lv_obj_set_style_text_font(ui_ListItem7, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem7, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem8 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_CLOSE, "Exit Settings");
    // lv_obj_set_style_text_font(ui_ListItem8, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_ListItem8, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // 添加额外的10个列表项
    ui_ListItem9 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_PLUS, "User Management");
    lv_obj_set_style_pad_all(ui_ListItem9, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem10 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_REFRESH, "Security Settings");
    lv_obj_set_style_pad_all(ui_ListItem10, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem11 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_BELL, "Notification Settings");
    lv_obj_set_style_pad_all(ui_ListItem11, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem12 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_PLUS, "Permission Management");
    lv_obj_set_style_pad_all(ui_ListItem12, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem13 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_PLUS, "Data Backup");
    lv_obj_set_style_pad_all(ui_ListItem13, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem14 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_REFRESH, "System Reset");
    lv_obj_set_style_pad_all(ui_ListItem14, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem15 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_EDIT, "Language Settings");
    lv_obj_set_style_pad_all(ui_ListItem15, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem16 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_PLUS, "Performance Monitor");
    lv_obj_set_style_pad_all(ui_ListItem16, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem17 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_PLUS, "Add Device");
    lv_obj_set_style_pad_all(ui_ListItem17, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_ListItem18 = lv_list_add_btn(ui_ListSetting, LV_SYMBOL_PLUS, "Help Center");
    lv_obj_set_style_pad_all(ui_ListItem18, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();
    /* Initialize display and LVGL */
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = 1,
        .flags = {
            .buff_spiram = true,
        }
    };
    bsp_display_start_with_config(&cfg);

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    bsp_display_lock(0);

    ui_setting_screen_init();

    bsp_display_unlock();
}
