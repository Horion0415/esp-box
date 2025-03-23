#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "math.h"
#include "esp_random.h"
#include "bsp/esp-bsp.h"

#include "eye_config.h"

static const char *TAG = "animated_eyes";

// 全局变量
esp_lcd_panel_handle_t lcd_panel[NUM_EYES];
esp_lcd_panel_io_handle_t lcd_io[NUM_EYES];

// 像素缓冲区
#define BUFFER_SIZE (BSP_LCD_H_RES * 100)
uint16_t *pbuffer = NULL;

// 眼睛结构体
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // 眨眼状态持续时间 (微秒)
  uint32_t startTime;   // 上次状态变化的时间 (微秒)
} eyeBlink;

struct {
  int16_t   tft_cs;     // 每个显示器的片选引脚
  eyeBlink  blink;      // 当前眨眼/眨眼状态
  int16_t   xposition;  // 眼睛图像的x位置
} eye[NUM_EYES];

uint64_t startTime;  // 用于FPS指示器

#if !defined(LIGHT_PIN) || (LIGHT_PIN < 0)
// 自主虹膜运动使用分形行为来模拟眼睛的主要反应和持续的较小调整
uint16_t oldIris = (IRIS_MIN + IRIS_MAX) / 2, newIris;
#endif

// 函数声明
void init_eyes(void);
void update_eye(void);
void draw_eye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT);
void frame(uint16_t iScale);
void split(int16_t startValue, int16_t endValue, uint64_t startTime, int32_t duration, int16_t range);
void user_loop(void);
int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max);

// 缓动函数查找表 - 用于平滑眼球运动
const uint8_t ease[] = {
    0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9, 11, 12, 13, 15,
    17, 18, 20, 22, 24, 26, 28, 30, 32, 35, 37, 39, 42, 44, 47, 49,
    52, 55, 58, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 97,
    100,103,106,109,113,116,119,122,125,128,131,134,137,140,143,146,
    149,152,155,158,160,163,166,168,171,173,176,178,181,183,185,187,
    190,192,194,196,198,200,202,204,205,207,209,210,212,213,215,216,
    218,219,220,221,222,224,225,226,226,227,228,229,230,230,231,232,
    232,233,234,234,235,235,235,236,236,236,237,237,237,238,238,238,
    238,238,239,239,239,239,239,239,239,239,239,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
    240,240,240,240,240,240,240,240,240,240,240,240,240,240,240, 240
};

// 初始化LCD面板
static esp_err_t init_lcd_panel(int eye_index) {
    ESP_LOGI(TAG, "初始化LCD面板 #%d", eye_index);
    
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = (BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT) * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(bsp_display_new(&bsp_disp_cfg, &lcd_panel[eye_index], &lcd_io[eye_index]));
    esp_lcd_panel_disp_on_off(lcd_panel[eye_index], true);
    bsp_display_backlight_on();
    
    return ESP_OK;
}

// 初始化眼睛
void init_eyes(void) {
    ESP_LOGI(TAG, "初始化眼睛对象");
    
    pbuffer = (uint16_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);

    // 根据config.h中的eyeInfo列表初始化眼睛对象
    for (uint8_t e = 0; e < NUM_EYES; e++) {
        ESP_LOGI(TAG, "创建显示器 #%d", e);
        
        eye[e].tft_cs = eyeInfo[e].select;
        eye[e].blink.state = NOBLINK;
        eye[e].xposition = eyeInfo[e].xposition;
        
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << eye[e].tft_cs),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(eye[e].tft_cs, 0);
        
        // 初始化LCD面板
        init_lcd_panel(e);
        
        // 如果定义了单独的眨眼引脚，也设置它
        if (eyeInfo[e].wink >= 0) {
            io_conf.pin_bit_mask = (1ULL << eyeInfo[e].wink);
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            gpio_config(&io_conf);
        }
    }
    
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
    // 为所有眼睛的眨眼引脚设置
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLINK_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
#endif
}

// 更新眼睛
void update_eye(void) {
#if defined(LIGHT_PIN) && (LIGHT_PIN >= 0) // 交互式虹膜
    int16_t v = 0; // 这里需要实现ADC读取
    // 使用ESP-IDF的ADC读取
    // adc1_config_width(ADC_WIDTH_BIT_12);
    // adc1_config_channel_atten(LIGHT_PIN, ADC_ATTEN_DB_11);
    // v = adc1_get_raw(LIGHT_PIN);
    
#ifdef LIGHT_PIN_FLIP
    v = 1023 - v;
#endif
    if (v < LIGHT_MIN) v = LIGHT_MIN;
    else if (v > LIGHT_MAX) v = LIGHT_MAX;
    v -= LIGHT_MIN;
    
#ifdef LIGHT_CURVE
    v = (int16_t)(pow((double)v / (double)(LIGHT_MAX - LIGHT_MIN),
                    LIGHT_CURVE) * (double)(LIGHT_MAX - LIGHT_MIN));
#endif
    
    v = map(v, 0, (LIGHT_MAX - LIGHT_MIN), IRIS_MAX, IRIS_MIN);
    
#ifdef IRIS_SMOOTH
    static int16_t irisValue = (IRIS_MIN + IRIS_MAX) / 2;
    irisValue = ((irisValue * 15) + v) / 16;
    frame(irisValue);
#else
    frame(v);
#endif

#else // 自主虹膜缩放
    newIris = esp_random() % (IRIS_MAX - IRIS_MIN + 1) + IRIS_MIN;
    split(oldIris, newIris, esp_timer_get_time(), 10000000L, IRIS_MAX - IRIS_MIN);
    oldIris = newIris;
#endif
}

// 绘制眼睛函数
void draw_eye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT) {
    uint32_t screenX, screenY, scleraXsave;
    int32_t irisX, irisY;
    uint16_t p;
    uint32_t d, a;
    uint32_t pixels = 0;
        
    scleraXsave = scleraX;
    irisY = scleraY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;
    
    // 眼睑图像对于两个显示器是左右交换的
    uint16_t lidX = 0;
    uint16_t dlidX = -1;
    if (e) dlidX = 1;
    
    for (screenY = 0; screenY < SCREEN_HEIGHT; screenY++, scleraY++, irisY++) {
        scleraX = scleraXsave;
        irisX = scleraXsave - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
        if (e) lidX = 0; else lidX = SCREEN_WIDTH - 1;
        
        for (screenX = 0; screenX < SCREEN_WIDTH; screenX++, scleraX++, irisX++, lidX += dlidX) {
            // 检查眼睑位置
            if ((lower[screenY * SCREEN_WIDTH + lidX] <= lT) ||
                (upper[screenY * SCREEN_WIDTH + lidX] <= uT)) {
                p = 0; // 眼睑覆盖的区域为黑色
            } 
            // 检查是否在虹膜区域内
            else if ((irisY < 0) || (irisY >= IRIS_HEIGHT) ||
                     (irisX < 0) || (irisX >= IRIS_WIDTH)) {
                // 不在虹膜区域内，使用巩膜颜色
                p = sclera[scleraY * SCLERA_WIDTH + scleraX];
            } else {
                // 在虹膜区域内，计算虹膜颜色
                p = polar[irisY * IRIS_WIDTH + irisX];
                d = (iScale * (p & 0x7F)) / 128; // 提取距离分量
                
                if (d < IRIS_MAP_HEIGHT) {
                    a = (IRIS_MAP_WIDTH * (p >> 7)) / 512; // 提取角度分量
                    p = iris[d * IRIS_MAP_WIDTH + a]; // 查找虹膜颜色
                } else {
                    // 超出虹膜范围，使用巩膜颜色
                    p = sclera[scleraY * SCLERA_WIDTH + scleraX];
                }
            }
            
            // 在ESP32中，可能需要交换字节顺序，取决于LCD控制器的要求
            // 有些控制器需要RGB565格式的高低字节交换
            // 如果您的显示器显示颜色不正确，可能需要调整这里
            uint16_t color = (p >> 8) | (p << 8); // 交换字节顺序
            // 或者直接使用: uint16_t color = p; // 如果不需要交换
            
            pbuffer[pixels++] = color;
            
            // 当缓冲区满时发送数据
            if (pixels >= BUFFER_SIZE) {
                // 计算当前行
                uint16_t currentRow = screenY - (pixels / SCREEN_WIDTH) + 1;
                
                // 发送像素数据到LCD
                esp_lcd_panel_draw_bitmap(lcd_panel[e], 
                                         eye[e].xposition, 
                                         currentRow, 
                                         eye[e].xposition + SCREEN_WIDTH, 
                                         screenY + 1, 
                                         pbuffer);
                pixels = 0;
            }
        }
    }
    
    // 发送剩余的像素数据（如果有）
    if (pixels) {
        uint16_t remainingRows = pixels / SCREEN_WIDTH;
        uint16_t startRow = SCREEN_HEIGHT - remainingRows;
        
        esp_lcd_panel_draw_bitmap(lcd_panel[e], 
                                 eye[e].xposition, 
                                 startRow, 
                                 eye[e].xposition + SCREEN_WIDTH, 
                                 SCREEN_HEIGHT, 
                                 pbuffer);
    }
}

// 帧处理函数
void frame(uint16_t iScale) {
    static uint32_t frames = 0;
    static uint8_t eyeIndex = 0;
    int16_t eyeX, eyeY;
    uint64_t t = esp_timer_get_time();
    
    // 每256帧显示一次FPS
    if (!(++frames & 255)) {
        float elapsed = (esp_timer_get_time() - startTime) / 1000000.0;
        if (elapsed) ESP_LOGI(TAG, "FPS: %d", (uint16_t)(frames / elapsed));
    }
    
    // 循环处理每只眼睛
    if (++eyeIndex >= NUM_EYES) eyeIndex = 0;
    
    // 自主眼球运动
    static bool eyeInMotion = false;
    static int16_t eyeOldX = 512, eyeOldY = 512, eyeNewX = 512, eyeNewY = 512;
    static uint64_t eyeMoveStartTime = 0;
    static int32_t eyeMoveDuration = 0;
    
    int32_t dt = t - eyeMoveStartTime;
    if (eyeInMotion) {
        // 眼球正在移动
        if (dt >= eyeMoveDuration) {
            // 移动完成，设置新的静止期
            eyeInMotion = false;
            eyeMoveDuration = esp_random() % 3000000; // 随机静止时间
            eyeMoveStartTime = t;
            eyeX = eyeOldX = eyeNewX;
            eyeY = eyeOldY = eyeNewY;
        } else {
            // 计算当前移动位置（使用缓动函数）
            int16_t e = ease[255 * dt / eyeMoveDuration] + 1;
            eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256);
            eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256);
        }
    } else {
        // 眼球静止
        eyeX = eyeOldX;
        eyeY = eyeOldY;
        if (dt > eyeMoveDuration) {
            // 静止时间结束，计算新的目标位置
            int16_t dx, dy;
            uint32_t d;
            do {
                // 生成随机目标位置，但确保在有效圆内
                eyeNewX = esp_random() % 1024;
                eyeNewY = esp_random() % 1024;
                dx = (eyeNewX * 2) - 1023;
                dy = (eyeNewY * 2) - 1023;
            } while ((d = (dx * dx + dy * dy)) > (1023 * 1023)); // 确保在圆内
            
            eyeMoveDuration = 72000 + esp_random() % 72000; // 随机移动时间
            eyeMoveStartTime = t;
            eyeInMotion = true;
        }
    }
    
    // 眨眼处理
#ifdef AUTOBLINK
    static uint64_t timeOfLastBlink = 0, timeToNextBlink = 0;
    
    // 检查是否到了下一次眨眼的时间
    if ((t - timeOfLastBlink) >= timeToNextBlink) {
        timeOfLastBlink = t;
        uint32_t blinkDuration = 36000 + esp_random() % 36000;
        
        // 为所有眼睛设置眨眼状态
        for (uint8_t e = 0; e < NUM_EYES; e++) {
            if (eye[e].blink.state == NOBLINK) {
                eye[e].blink.state = ENBLINK;
                eye[e].blink.startTime = t;
                eye[e].blink.duration = blinkDuration;
            }
        }
        // 设置下一次眨眼的时间
        timeToNextBlink = blinkDuration * 3 + esp_random() % 4000000;
    }
#endif
    
    // 处理当前眼睛的眨眼状态
    if (eye[eyeIndex].blink.state) {
        // 眼睛正在眨眼
        if ((t - eye[eyeIndex].blink.startTime) >= eye[eyeIndex].blink.duration) {
            // 当前眨眼阶段完成
            if ((eye[eyeIndex].blink.state == ENBLINK) && (
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
                (gpio_get_level(BLINK_PIN) == 0) ||
#endif
                ((eyeInfo[eyeIndex].wink >= 0) &&
                 gpio_get_level(eyeInfo[eyeIndex].wink) == 0))) {
                // 如果按钮仍然按下，保持眼睛关闭
            } else {
                // 进入下一个眨眼阶段或完成眨眼
                if (++eye[eyeIndex].blink.state > DEBLINK) {
                    eye[eyeIndex].blink.state = NOBLINK;
                } else {
                    eye[eyeIndex].blink.duration *= 2; // 睁眼通常比闭眼慢
                    eye[eyeIndex].blink.startTime = t;
                }
            }
        }
    } else {
        // 眼睛没有眨眼，检查是否应该开始眨眼
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
        if (gpio_get_level(BLINK_PIN) == 0) {
            // 眨眼按钮被按下，开始眨眼
            uint32_t blinkDuration = 36000 + esp_random() % 36000;
            for (uint8_t e = 0; e < NUM_EYES; e++) {
                if (eye[e].blink.state == NOBLINK) {
                    eye[e].blink.state = ENBLINK;
                    eye[e].blink.startTime = t;
                    eye[e].blink.duration = blinkDuration;
                }
            }
        } else
#endif
        // 检查单眼眨眼按钮
        if ((eyeInfo[eyeIndex].wink >= 0) &&
            (gpio_get_level(eyeInfo[eyeIndex].wink) == 0)) {
            eye[eyeIndex].blink.state = ENBLINK;
            eye[eyeIndex].blink.startTime = t;
            eye[eyeIndex].blink.duration = 45000 + esp_random() % 45000;
        }
    }
    
    // 将眼睛X/Y位置(0-1023)缩放到drawEye()使用的像素单位
    eyeX = map(eyeX, 0, 1023, 0, SCLERA_WIDTH - 128);
    eyeY = map(eyeY, 0, 1023, 0, SCLERA_HEIGHT - 128);
    
    // 水平位置偏移，使眼睛略微交叉
    if (NUM_EYES > 1) {
        if (eyeIndex == 1) eyeX += 4;
        else eyeX -= 4;
    }
    if (eyeX > (SCLERA_WIDTH - 128)) eyeX = (SCLERA_WIDTH - 128);
    
    // 眼睑渲染
    static uint8_t uThreshold = 128;
    uint8_t lThreshold, n;
    
#ifdef TRACKING
    // 眼睑跟踪瞳孔位置
    int16_t sampleX = SCLERA_WIDTH / 2 - (eyeX / 2);
    int16_t sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
    
    if (sampleY < 0) n = 0;
    else {
        // 直接从数组读取，不使用pgm_read_byte
        n = (upper[sampleY * SCREEN_WIDTH + sampleX] +
             upper[sampleY * SCREEN_WIDTH + (SCREEN_WIDTH - 1 - sampleX)]) / 2;
    }
    
    // 平滑眼睑移动
    uThreshold = (uThreshold * 3 + n) / 4;
    lThreshold = 254 - uThreshold;
#else
    uThreshold = lThreshold = 0;
#endif
    
    // 处理眨眼动画
    if (eye[eyeIndex].blink.state) {
        uint32_t s = (t - eye[eyeIndex].blink.startTime);
        if (s >= eye[eyeIndex].blink.duration) s = 255;
        else s = 255 * s / eye[eyeIndex].blink.duration;
        
        // 根据眨眼状态计算眼睑位置
        s = (eye[eyeIndex].blink.state == DEBLINK) ? 1 + s : 256 - s;
        n = (uThreshold * s + 254 * (257 - s)) / 256;
        lThreshold = (lThreshold * s + 254 * (257 - s)) / 256;
    } else {
        n = uThreshold;
    }
    
    // 传递所有派生值到眼睛渲染函数
    draw_eye(eyeIndex, iScale, eyeX, eyeY, n, lThreshold);
    
    // 在处理完所有眼睛后调用用户循环函数
    if (eyeIndex == (NUM_EYES - 1)) {
        user_loop();
    }
}

// 自主虹膜缩放
#if !defined(LIGHT_PIN) || (LIGHT_PIN < 0)
void split(int16_t startValue, int16_t endValue, uint64_t startTime, int32_t duration, int16_t range) {
    if (range >= 8) {
        range /= 2;
        duration /= 2;
        int16_t midValue = (startValue + endValue - range) / 2 + esp_random() % range;
        uint64_t midTime = startTime + duration;
        split(startValue, midValue, startTime, duration, range);
        split(midValue, endValue, midTime, duration, range);
    } else {
        int32_t dt;
        int16_t v;
        while ((dt = (esp_timer_get_time() - startTime)) < duration) {
            v = startValue + (((endValue - startValue) * dt) / duration);
            if (v < IRIS_MIN) v = IRIS_MIN;
            else if (v > IRIS_MAX) v = IRIS_MAX;
            frame(v);
        }
    }
}
#endif

// 辅助函数：映射值
int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void user_loop(void) {
    // 用户可以在这里添加自定义循环代码
}

// 主函数
void app_main(void) {
    ESP_LOGI(TAG, "启动动画眼睛程序");
    
    // 初始化眼睛
    init_eyes();
    
    // 记录开始时间
    startTime = esp_timer_get_time();

    // 主循环
    while (1) {
        update_eye();
        vTaskDelay(50 / portTICK_PERIOD_MS); // 给其他任务一些时间
    }
}