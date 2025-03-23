#pragma once

// Graphic settings (eye appearance) -----------------------------------

// If using a single eye, you might want to enable the next line, which uses a simpler
// "football-shaped" eye that's left/right symmetrical. The default shape includes
// the caruncle (tear duct), creating distinct left/right eyes.
#define SYMMETRICAL_EYELID

// Enable ONE of these #includes -- huge graphics tables for various eyes:
#include "defaultEye.h"      // Standard human-ish brown eye -OR-
//#include "dragonEye.h"     // Slit pupil dragon/demon eye -OR-
//#include "noScleraEye.h"   // Large iris, no sclera -OR-
//#include "goatEye.h"       // Horizontal pupil goat/Krampus eye -OR-
//#include "newtEye.h"       // Newt eye -OR-
//#include "terminatorEye.h" // Terminator eye!
//#include "catEye.h"        // Cartoon cat eye (flat "2D" colors)
//#include "owlEye.h"        // Owl eye (tracking disabled)
//#include "naugaEye.h"      // Nauga eye (tracking disabled)
//#include "doeEye.h"        // Cartoon deer eye (tracking disabled)

// Display hardware settings (screen type and connection) -------------------
#define TFT_COUNT 1        // Number of screens (1 or 2)
#define TFT1_CS 5          // TFT 1 chip select pin
#define TFT2_CS -1         // TFT 2 chip select pin
#define TFT_1_ROT 1        // TFT 1 rotation
#define TFT_2_ROT 1        // TFT 2 rotation
#define EYE_1_XPOSITION  0         // x offset of eye 1 image on display
#define EYE_2_XPOSITION  320 - 128 // x offset of eye 2 image on display

// Eye list ----------------------------------------------------------------
#define NUM_EYES 1 // Number of eyes to display (1 or 2)

#define BLINK_PIN   -1 // Manual blink button pin (both eyes)
#define LH_WINK_PIN -1 // Left wink pin (set to -1 for no pin)
#define RH_WINK_PIN -1 // Right wink pin (set to -1 for no pin)

// Input settings (for controlling eye motion) -----------------------------

#define TRACKING        1   // If defined, eyelids track pupil
#define AUTOBLINK       1   // If defined, eyes also blink autonomously

#define IRIS_SMOOTH         // If enabled, filter input from IRIS_PIN
#define IRIS_MIN       90   // Iris size in brightest light (0-1023)
#define IRIS_MAX      130   // Iris size in darkest light (0-1023)

// Blink state definitions
#define NOBLINK 0       // Not currently blinking
#define ENBLINK 1       // Eyelid is currently closing
#define DEBLINK 2       // Eyelid is currently opening

typedef struct {
  int8_t  select;
  int8_t  wink;
  uint8_t rotation;
  int16_t xposition;
} eyeInfo_t;

#if (NUM_EYES == 2)
  eyeInfo_t eyeInfo[] = {
    { TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION }, // Left eye chip select and wink pin, rotation and offset
    { TFT2_CS, RH_WINK_PIN, TFT_2_ROT, EYE_2_XPOSITION }, // Right eye chip select and wink pin, rotation and offset
  };
#else
  eyeInfo_t eyeInfo[] = {
    { TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION }, // Eye chip select and wink pin, rotation and offset
  };
#endif