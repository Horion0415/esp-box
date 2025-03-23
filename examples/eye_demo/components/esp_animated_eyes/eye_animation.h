#pragma once

#include "sdkconfig.h"
#include <stdint.h>
#include <stdbool.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

// If using a single eye, you might want to enable the next line, which uses a simpler
// "football-shaped" eye that's left/right symmetrical. The default shape includes
// the caruncle (tear duct), creating distinct left/right eyes.
#if CONFIG_SYMMETRICAL_EYELID
#define SYMMETRICAL_EYELID
#endif
#if CONFIG_ENABLE_TRACKING
#define TRACKING
#endif
#if CONFIG_ENABLE_AUTOBLINK
#define AUTOBLINK
#endif

// Eye config
#define BUFFER_SIZE          (CONFIG_EYE_BUFFER_SIZE) // Buffer size for eye graphics

// Eye movement control modes
typedef enum {
    EYE_MODE_FIXED,       // Eye stays at fixed position
    EYE_MODE_AUTO,        // Eye moves automatically (random)
    EYE_MODE_CUSTOM_PATH  // Eye follows custom path
} eye_movement_mode_t;

// Eye blink structure
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of current blink state (microseconds)
  uint32_t startTime;   // Time of last state change (microseconds)
} eyeBlink;

// Eye structure
typedef struct {
  eyeBlink  blink;      // Current blink/wink state
  int16_t   xposition;  // x position of eye image
} eye_t;

// Eye info structure
typedef struct {
  int8_t  wink;
  uint8_t rotation;
  int16_t xposition;
} eyeInfo_t;

// Initialize eye animation with existing LCD panels
void eye_animation_init(esp_lcd_panel_handle_t *panels, eyeInfo_t *eye_configs, uint8_t num_eyes);

// Start eye animation
void eye_animation_start(void);

// Deinitialize eye animation
void eye_animation_deinit(void);

// Update eye animation
void eye_animation_update(void);

// Set eye position (coordinates range: 0-1023)
void eye_set_position(int16_t x, int16_t y);

// Get current eye position
void eye_get_position(int16_t *x, int16_t *y);

// User customizable loop function
void eye_user_loop(void);

// Get eye structures array
eye_t* eye_get_eyes(void);

// Get pixel buffer
uint16_t* eye_get_pixel_buffer(void);

// Get number of eyes
uint8_t eye_get_count(void);

// Set eye to fixed position
void eye_set_fixed_position(int16_t x, int16_t y);

// Set eye to auto movement mode
void eye_set_auto_movement(void);

// Set custom path for eye to follow
void eye_set_custom_path(int16_t positions[][2], int num_positions, uint32_t hold_time_ms);