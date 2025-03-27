#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_lcd_panel_ops.h"
#include "math.h"

#include "eye_animation.h"

// Enable ONE of these #includes -- huge graphics tables for various eyes:
#if CONFIG_EYE_TYPE_DEFAULT
#include "defaultEye.h"      // Standard human-ish brown eye -OR-
#elif CONFIG_EYE_TYPE_DRAGON
#include "dragonEye.h"     // Slit pupil dragon/demon eye -OR-
#elif CONFIG_EYE_TYPE_NO_SCLERA
#include "noScleraEye.h"   // Large iris, no sclera -OR-
#elif CONFIG_EYE_TYPE_GOAT
#include "goatEye.h"       // Horizontal pupil goat/Krampus eye -OR-
#elif CONFIG_EYE_TYPE_NEWT
#include "newtEye.h"       // Newt eye -OR-
#elif CONFIG_EYE_TYPE_TERMINATOR
#include "terminatorEye.h" // Terminator eye!
#elif CONFIG_EYE_TYPE_CAT
#include "catEye.h"        // Cartoon cat eye (flat "2D" colors)
#elif CONFIG_EYE_TYPE_OWL
#include "owlEye.h"        // Owl eye (tracking disabled)
#elif CONFIG_EYE_TYPE_NAUGA
#include "naugaEye.h"      // Nauga eye (tracking disabled)
#elif CONFIG_EYE_TYPE_DOE
#include "doeEye.h"        // Cartoon deer eye (tracking disabled)
#endif

#define MAX_EYES  2

// Blink state definitions
#define NOBLINK 0       // Not currently blinking
#define ENBLINK 1       // Eyelid is currently closing
#define DEBLINK 2       // Eyelid is currently opening

#define IRIS_MIN       90   // Iris size in brightest light (0-1023)
#define IRIS_MAX      130   // Iris size in darkest light (0-1023)

#define EYE_MOVE_DURATION 500000  

static const char *TAG = "eye_animation";

// Global control variables
static eye_movement_mode_t eye_mode = EYE_MODE_AUTO;
static int16_t fixed_position_x = 512;
static int16_t fixed_position_y = 512;

// Global variables
static esp_lcd_panel_handle_t *lcd_panel;

// Pixel buffer
static uint16_t *pbuffer = NULL;

// Eye array
static eye_t eye[MAX_EYES];

// Eye count
static uint8_t num_eyes = 0;

// Eye configurations
static eyeInfo_t *eye_configs = NULL;

// Eye movement variables
static bool eyeInMotion = false;
static int16_t eyeOldX = 512, eyeOldY = 512, eyeNewX = 512, eyeNewY = 512;
static uint64_t eyeMoveStartTime = 0;
static int32_t eyeMoveDuration = 0;

// Timing variables
static uint64_t startTime;  // For FPS indicator

// Autonomous iris motion uses fractal behavior to simulate both the major
// reaction and ongoing smaller adjustments
static uint16_t oldIris = (IRIS_MIN + IRIS_MAX) / 2, newIris;

// Task handles
static TaskHandle_t animation_task_handle = NULL;
static TaskHandle_t control_task_handle = NULL;

// Easing function lookup table - for smooth eye movement
static const uint8_t ease[] = {
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

// Function declarations
static void draw_eye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT);
static void frame(uint16_t iScale);
static void split(int16_t startValue, int16_t endValue, uint64_t startTime, int32_t duration, int16_t range);
static void process_eye_movement(uint64_t t, int16_t *eyeX, int16_t *eyeY);
static int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max);

// Initialize eye animation with existing LCD panels
void eye_animation_init(esp_lcd_panel_handle_t *panels, eyeInfo_t *configs, uint8_t eyes_count) {
    ESP_LOGI(TAG, "Initializing eye animation with %d eyes", eyes_count);
    
    // Store the provided LCD panel and IO handles
    lcd_panel = panels;

    // Store the provided eye configurations
    num_eyes = eyes_count;
    
    // Allocate memory for eye configurations
    eye_configs = (eyeInfo_t *)malloc(num_eyes * sizeof(eyeInfo_t));
    if (!eye_configs) {
        ESP_LOGE(TAG, "Failed to allocate eye configs");
        return;
    }
    memcpy(eye_configs, configs, num_eyes * sizeof(eyeInfo_t));

    // Allocate pixel buffer
    pbuffer = (uint16_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!pbuffer) {
        ESP_LOGE(TAG, "Failed to allocate pixel buffer");
        return;
    }

    // Initialize eye structures
    for (uint8_t e = 0; e < num_eyes; e++) {
        ESP_LOGI(TAG, "Initializing eye #%d", e);
        
        eye[e].blink.state = NOBLINK;
        eye[e].xposition = eye_configs[e].xposition;
        
        // Setup wink pin for all eyes
        if (eye_configs[e].wink >= 0) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << eye_configs[e].wink),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            gpio_config(&io_conf);
        }
    }

    // Record start time
    startTime = esp_timer_get_time();
}

// Update eye animation
void eye_animation_update(void) {
    newIris = esp_random() % (IRIS_MAX - IRIS_MIN + 1) + IRIS_MIN;
    split(oldIris, newIris, esp_timer_get_time(), 10000000L, IRIS_MAX - IRIS_MIN);
    oldIris = newIris;
}

void eye_set_position(int16_t x, int16_t y) 
{
    x = (x < 0) ? 0 : ((x > 1023) ? 1023 : x);
    y = (y < 0) ? 0 : ((y > 1023) ? 1023 : y);
    
    eye_mode = EYE_MODE_FIXED;
    fixed_position_x = x;
    fixed_position_y = y;
    
    eyeNewX = x;
    eyeNewY = y;
    
    eyeInMotion = true;
    eyeMoveStartTime = esp_timer_get_time();
    eyeMoveDuration = EYE_MOVE_DURATION; 
    
    ESP_LOGI(TAG, "Eye position set to fixed position (%d, %d)", x, y);
}

// Get current eye position
void eye_get_position(int16_t *x, int16_t *y) {
    if (x) *x = eyeOldX;
    if (y) *y = eyeOldY;
}

// Process eye movement
static void process_eye_movement(uint64_t t, int16_t *eyeX, int16_t *eyeY) {
    if (eye_mode == EYE_MODE_FIXED) {
        *eyeX = fixed_position_x;
        *eyeY = fixed_position_y;
        
        // 如果眼睛正在移动到固定位置，继续移动动画
        if (eyeInMotion) {
            int32_t dt = t - eyeMoveStartTime;
            if (dt >= eyeMoveDuration) {
                // 移动完成
                eyeInMotion = false;
                eyeOldX = eyeNewX = fixed_position_x;
                eyeOldY = eyeNewY = fixed_position_y;
            } else {
                // 计算当前移动位置（使用缓动函数）
                int16_t e = ease[255 * dt / eyeMoveDuration] + 1;
                *eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256);
                *eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256);
            }
        }
        return;
    }
    
    // 以下是自动模式的处理逻辑
    int32_t dt = t - eyeMoveStartTime;
    if (eyeInMotion) {
        // 眼睛正在移动
        if (dt >= eyeMoveDuration) {
            // 移动完成，设置新的休息期
            eyeInMotion = false;
            eyeMoveDuration = esp_random() % 3000000; // 随机休息时间
            eyeMoveStartTime = t;
            *eyeX = eyeOldX = eyeNewX;
            *eyeY = eyeOldY = eyeNewY;
        } else {
            // 计算当前移动位置（使用缓动函数）
            int16_t e = ease[255 * dt / eyeMoveDuration] + 1;
            *eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256);
            *eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256);
        }
    } else {
        // 眼睛处于休息状态
        *eyeX = eyeOldX;
        *eyeY = eyeOldY;
        
        if (dt > eyeMoveDuration) {
            // 休息期结束，计算新的目标位置
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
}

// Draw eye function
static void draw_eye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT) {
    uint32_t screenX, screenY, scleraXsave;
    int32_t irisX, irisY;
    uint16_t p;
    uint32_t d, a;
    uint32_t pixels = 0;
        
    scleraXsave = scleraX;
    irisY = scleraY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;
    
    // Eyelid images are mirrored for the two displays
    uint16_t lidX = 0;
    uint16_t dlidX = -1;
    if (e) dlidX = 1;
    
    for (screenY = 0; screenY < SCREEN_HEIGHT; screenY++, scleraY++, irisY++) {
        scleraX = scleraXsave;
        irisX = scleraXsave - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
        if (e) lidX = 0; else lidX = SCREEN_WIDTH - 1;
        
        for (screenX = 0; screenX < SCREEN_WIDTH; screenX++, scleraX++, irisX++, lidX += dlidX) {
            // Check eyelid position
            if ((lower[screenY * SCREEN_WIDTH + lidX] <= lT) ||
                (upper[screenY * SCREEN_WIDTH + lidX] <= uT)) {
                p = 0; // Black for areas covered by eyelids
            } 
            // Check if within iris area
            else if ((irisY < 0) || (irisY >= IRIS_HEIGHT) ||
                     (irisX < 0) || (irisX >= IRIS_WIDTH)) {
                // Not in iris area, use sclera color
                p = sclera[scleraY * SCLERA_WIDTH + scleraX];
            } else {
                // In iris area, calculate iris color
                p = polar[irisY * IRIS_WIDTH + irisX];
                d = (iScale * (p & 0x7F)) / 128; // Extract distance component
                
                if (d < IRIS_MAP_HEIGHT) {
                    a = (IRIS_MAP_WIDTH * (p >> 7)) / 512; // Extract angle component
                    p = iris[d * IRIS_MAP_WIDTH + a]; // Look up iris color
                } else {
                    // Outside iris range, use sclera color
                    p = sclera[scleraY * SCLERA_WIDTH + scleraX];
                }
            }
            
            // On ESP32, byte swapping may be needed depending on LCD controller
            uint16_t color = (p >> 8) | (p << 8); // Swap byte order
            
            pbuffer[pixels++] = color;
            
            // Send data when buffer is full
            if (pixels >= BUFFER_SIZE) {
                // Calculate current row
                uint16_t currentRow = screenY - (pixels / SCREEN_WIDTH) + 1;
                
                // Send pixel data to LCD
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
    
    // Send remaining pixel data (if any)
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

// Frame processing function
static void frame(uint16_t iScale) {
    static uint32_t frames = 0;
    static uint8_t eyeIndex = 0;
    int16_t eyeX, eyeY;
    uint64_t t = esp_timer_get_time();
    
    // Display FPS every 256 frames
    if (!(++frames & 255)) {
        float elapsed = (esp_timer_get_time() - startTime) / 1000000.0;
        if (elapsed) ESP_LOGI(TAG, "FPS: %d", (uint16_t)(frames / elapsed));
    }
    
    // Process each eye in turn
    if (++eyeIndex >= num_eyes) eyeIndex = 0;
    
    // Process eye movement
    process_eye_movement(t, &eyeX, &eyeY);
    
    // Blink processing
#ifdef AUTOBLINK
    static uint64_t timeOfLastBlink = 0, timeToNextBlink = 0;
    
    // Check if it's time for next blink
    if ((t - timeOfLastBlink) >= timeToNextBlink) {
        timeOfLastBlink = t;
        uint32_t blinkDuration = 36000 + esp_random() % 36000;
        
        // Set blink state for all eyes
        for (uint8_t e = 0; e < num_eyes; e++) {
            if (eye[e].blink.state == NOBLINK) {
                eye[e].blink.state = ENBLINK;
                eye[e].blink.startTime = t;
                eye[e].blink.duration = blinkDuration;
            }
        }
        // Set time to next blink
        timeToNextBlink = blinkDuration * 3 + esp_random() % 4000000;
    }
#endif
    
    // Process current eye's blink state
    if (eye[eyeIndex].blink.state) {
        // Eye is blinking
        if ((t - eye[eyeIndex].blink.startTime) >= eye[eyeIndex].blink.duration) {
            // Current blink phase complete
            if ((eye[eyeIndex].blink.state == ENBLINK) && (
                ((eye_configs[eyeIndex].wink >= 0) &&
                 gpio_get_level(eye_configs[eyeIndex].wink) == 0))) {
                // If button still pressed, keep eye closed
            } else {
                // Move to next blink phase or complete blink
                if (++eye[eyeIndex].blink.state > DEBLINK) {
                    eye[eyeIndex].blink.state = NOBLINK;
                } else {
                    eye[eyeIndex].blink.duration *= 2; // Opening is usually slower than closing
                    eye[eyeIndex].blink.startTime = t;
                }
            }
        }
    } else {
        // Check single-eye wink button
        if ((eye_configs[eyeIndex].wink >= 0) &&
            (gpio_get_level(eye_configs[eyeIndex].wink) == 0)) {
            eye[eyeIndex].blink.state = ENBLINK;
            eye[eyeIndex].blink.startTime = t;
            eye[eyeIndex].blink.duration = 45000 + esp_random() % 45000;
        }
    }
    
    // Scale eye X/Y position (0-1023) to pixel units used by drawEye()
    eyeX = map(eyeX, 0, 1023, 0, SCLERA_WIDTH - 128);
    eyeY = map(eyeY, 0, 1023, 0, SCLERA_HEIGHT - 128);
    
    // Horizontal position offset so eyes are slightly crossed
    if (num_eyes > 1) {
        if (eyeIndex == 1) eyeX += 4;
        else eyeX -= 4;
    }
    if (eyeX > (SCLERA_WIDTH - 128)) eyeX = (SCLERA_WIDTH - 128);
    
    // Eyelid rendering
    static uint8_t uThreshold = 128;
    uint8_t lThreshold, n;
    
#ifdef TRACKING
    // Eyelids track pupil position
    int16_t sampleX = SCLERA_WIDTH / 2 - (eyeX / 2);
    int16_t sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
    
    if (sampleY < 0) n = 0;
    else {
        n = (upper[sampleY * SCREEN_WIDTH + sampleX] +
             upper[sampleY * SCREEN_WIDTH + (SCREEN_WIDTH - 1 - sampleX)]) / 2;
    }
    
    // Smooth eyelid movement
    uThreshold = (uThreshold * 3 + n) / 4;
    lThreshold = 254 - uThreshold;
#else
    uThreshold = lThreshold = 0;
#endif
    
    // Process blink animation
    if (eye[eyeIndex].blink.state) {
        uint32_t s = (t - eye[eyeIndex].blink.startTime);
        if (s >= eye[eyeIndex].blink.duration) s = 255;
        else s = 255 * s / eye[eyeIndex].blink.duration;
        
        // Calculate eyelid position based on blink state
        s = (eye[eyeIndex].blink.state == DEBLINK) ? 1 + s : 256 - s;
        n = (uThreshold * s + 254 * (257 - s)) / 256;
        lThreshold = (lThreshold * s + 254 * (257 - s)) / 256;
    } else {
        n = uThreshold;
    }
    
    // Pass all derived values to eye rendering function
    draw_eye(eyeIndex, iScale, eyeX, eyeY, n, lThreshold);
    
    // Call user loop function after processing all eyes
    if (eyeIndex == (num_eyes - 1)) {
        eye_user_loop();
    }
}

// Split function for iris animation
static void split(int16_t startValue, int16_t endValue, uint64_t startTime, int32_t duration, int16_t range) {
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

    vTaskDelay(10 / portTICK_PERIOD_MS); // Give other tasks some time
}

// Helper function: map value from one range to another
static int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// User loop function - can be customized by the user
void eye_user_loop(void) {
    // User can add custom loop code here
}

// Eye animation task
static void eye_animation_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting eye animation task");
    
    while (1) {
        eye_animation_update();
    }
}

// 修改 eye_set_auto_movement 函数
void eye_set_auto_movement(void) {
    eye_mode = EYE_MODE_AUTO;
    eyeInMotion = false;
    eyeMoveStartTime = esp_timer_get_time();
    eyeMoveDuration = 0;  
    ESP_LOGI(TAG, "Eye set to auto movement mode");
}

void eye_animation_start(void) 
{
    ESP_LOGI(TAG, "Starting eye animation");
    
    xTaskCreate(eye_animation_task, "eye_animation", 4096, NULL, 5, &animation_task_handle);
    
    ESP_LOGI(TAG, "Eye animation task created");
}

// Stop eye animation
void eye_animation_stop(void) {
    ESP_LOGI(TAG, "Stopping eye animation");
    
    // Stop eye animation task
    if (animation_task_handle != NULL) {
        vTaskDelete(animation_task_handle);
        animation_task_handle = NULL;
    }
    
    // Stop eye control task
    if (control_task_handle != NULL) {
        vTaskDelete(control_task_handle);
        control_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "Eye animation tasks stopped");
}

void eye_animation_deinit(void) 
{
    if (animation_task_handle != NULL) {
        vTaskDelete(animation_task_handle);
        animation_task_handle = NULL;
    }
    
    if (eye_configs) {
        free(eye_configs);
        eye_configs = NULL;
    }
    
    if (pbuffer) {
        free(pbuffer);
        pbuffer = NULL;
    }
}

// Getter functions for accessing internal data
eye_t* eye_get_eyes(void) {
    return eye;
}

uint16_t* eye_get_pixel_buffer(void) {
    return pbuffer;
}

uint8_t eye_get_count(void) {
    return num_eyes;
}