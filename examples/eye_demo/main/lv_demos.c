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

// Global variables
esp_lcd_panel_handle_t lcd_panel[NUM_EYES];
esp_lcd_panel_io_handle_t lcd_io[NUM_EYES];

// Pixel buffer
#define BUFFER_SIZE (BSP_LCD_H_RES * 100)
uint16_t *pbuffer = NULL;

// Eye structure
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of current blink state (microseconds)
  uint32_t startTime;   // Time of last state change (microseconds)
} eyeBlink;

struct {
  int16_t   tft_cs;     // Chip select pin for each display
  eyeBlink  blink;      // Current blink/wink state
  int16_t   xposition;  // x position of eye image
} eye[NUM_EYES];

uint64_t startTime;  // For FPS indicator

// Autonomous iris motion uses fractal behavior to simulate both the major
// reaction and ongoing smaller adjustments
uint16_t oldIris = (IRIS_MIN + IRIS_MAX) / 2, newIris;

// Function declarations
void init_eyes(void);
void update_eye(void);
void draw_eye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT);
void frame(uint16_t iScale);
void split(int16_t startValue, int16_t endValue, uint64_t startTime, int32_t duration, int16_t range);
void user_loop(void);
int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max);

// Easing function lookup table - for smooth eye movement
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

void init_eyes(void) {
    ESP_LOGI(TAG, "Init eyes");
    
    pbuffer = (uint16_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);

    for (uint8_t e = 0; e < NUM_EYES; e++) {
        ESP_LOGI(TAG, "Create display #%d", e);
        
        eye[e].tft_cs = eyeInfo[e].select;
        eye[e].blink.state = NOBLINK;
        eye[e].xposition = eyeInfo[e].xposition;
        
        // Init cs pin
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << eye[e].tft_cs),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(eye[e].tft_cs, 0);
        
        // Init lcd panel
        init_lcd_panel(e);
        
        // If wink pin is defined, set it
        if (eyeInfo[e].wink >= 0) {
            io_conf.pin_bit_mask = (1ULL << eyeInfo[e].wink);
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            gpio_config(&io_conf);
        }
    }
    
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
    // Setup blink pin for all eyes
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

// Update eye
void update_eye(void) {
    newIris = esp_random() % (IRIS_MAX - IRIS_MIN + 1) + IRIS_MIN;
    split(oldIris, newIris, esp_timer_get_time(), 10000000L, IRIS_MAX - IRIS_MIN);
    oldIris = newIris;
}

// Draw eye function
void draw_eye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT) {
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
            // Some controllers need RGB565 format with high/low bytes swapped
            // Adjust this if your display shows incorrect colors
            uint16_t color = (p >> 8) | (p << 8); // Swap byte order
            // Or use directly: uint16_t color = p; // If no swap needed
            
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
void frame(uint16_t iScale) {
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
    if (++eyeIndex >= NUM_EYES) eyeIndex = 0;
    
    // Autonomous eye movement
    static bool eyeInMotion = false;
    static int16_t eyeOldX = 512, eyeOldY = 512, eyeNewX = 512, eyeNewY = 512;
    static uint64_t eyeMoveStartTime = 0;
    static int32_t eyeMoveDuration = 0;
    
    int32_t dt = t - eyeMoveStartTime;
    if (eyeInMotion) {
        // Eye is in motion
        if (dt >= eyeMoveDuration) {
            // Movement complete, set new rest period
            eyeInMotion = false;
            eyeMoveDuration = esp_random() % 3000000; // Random rest time
            eyeMoveStartTime = t;
            eyeX = eyeOldX = eyeNewX;
            eyeY = eyeOldY = eyeNewY;
        } else {
            // Calculate current movement position (using easing function)
            int16_t e = ease[255 * dt / eyeMoveDuration] + 1;
            eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256);
            eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256);
        }
    } else {
        // Eye is at rest
        eyeX = eyeOldX;
        eyeY = eyeOldY;
        if (dt > eyeMoveDuration) {
            // Rest period over, calculate new target position
            int16_t dx, dy;
            uint32_t d;
            do {
                // Generate random target position, but ensure within valid circle
                eyeNewX = esp_random() % 1024;
                eyeNewY = esp_random() % 1024;
                dx = (eyeNewX * 2) - 1023;
                dy = (eyeNewY * 2) - 1023;
            } while ((d = (dx * dx + dy * dy)) > (1023 * 1023)); // Ensure within circle
            
            eyeMoveDuration = 72000 + esp_random() % 72000; // Random movement time
            eyeMoveStartTime = t;
            eyeInMotion = true;
        }
    }
    
    // Blink processing
#ifdef AUTOBLINK
    static uint64_t timeOfLastBlink = 0, timeToNextBlink = 0;
    
    // Check if it's time for next blink
    if ((t - timeOfLastBlink) >= timeToNextBlink) {
        timeOfLastBlink = t;
        uint32_t blinkDuration = 36000 + esp_random() % 36000;
        
        // Set blink state for all eyes
        for (uint8_t e = 0; e < NUM_EYES; e++) {
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
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
                (gpio_get_level(BLINK_PIN) == 0) ||
#endif
                ((eyeInfo[eyeIndex].wink >= 0) &&
                 gpio_get_level(eyeInfo[eyeIndex].wink) == 0))) {
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
        // Eye not blinking, check if it should start
#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
        if (gpio_get_level(BLINK_PIN) == 0) {
            // Blink button pressed, start blink
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
        // Check single-eye wink button
        if ((eyeInfo[eyeIndex].wink >= 0) &&
            (gpio_get_level(eyeInfo[eyeIndex].wink) == 0)) {
            eye[eyeIndex].blink.state = ENBLINK;
            eye[eyeIndex].blink.startTime = t;
            eye[eyeIndex].blink.duration = 45000 + esp_random() % 45000;
        }
    }
    
    // Scale eye X/Y position (0-1023) to pixel units used by drawEye()
    eyeX = map(eyeX, 0, 1023, 0, SCLERA_WIDTH - 128);
    eyeY = map(eyeY, 0, 1023, 0, SCLERA_HEIGHT - 128);
    
    // Horizontal position offset so eyes are slightly crossed
    if (NUM_EYES > 1) {
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
        // Read directly from array, not using pgm_read_byte
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
    if (eyeIndex == (NUM_EYES - 1)) {
        user_loop();
    }
}

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

// Helper function: map value
int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void user_loop(void) {
    // User can add custom loop code here
}

// Main function
void app_main(void) {
    ESP_LOGI(TAG, "Start eye animation");
    
    // Init eyes
    init_eyes();
    
    // Record start time
    startTime = esp_timer_get_time();

    // Main loop
    while (1) {
        update_eye();
        vTaskDelay(50 / portTICK_PERIOD_MS); // Give other tasks some time
    }
}