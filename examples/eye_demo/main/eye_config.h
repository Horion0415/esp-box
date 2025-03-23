#pragma once

// 图形设置 (眼睛外观) -----------------------------------

// 如果使用单眼，您可能想启用下一行，它使用更简单的"足球形"眼睛，左右对称。
// 默认形状包括泪阜，创建不同的左右眼。

//#define SYMMETRICAL_EYELID

// 启用以下一个 #include -- 各种眼睛的巨大图形表:
#include "defaultEye.h"      // 标准人类棕色眼睛 -或-
//#include "dragonEye.h"     // 狭缝瞳孔火龙/恶魔眼 -或-
//#include "noScleraEye.h"   // 大虹膜，无巩膜 -或-
//#include "goatEye.h"       // 水平瞳孔山羊/克兰普斯眼 -或-
//#include "newtEye.h"       // 蝾螈眼 -或-
//#include "terminatorEye.h" // 终结者眼!
//#include "catEye.h"        // 卡通猫眼 (平面"2D"颜色)
//#include "owlEye.h"        // 猫头鹰眼 (禁用跟踪)
//#include "naugaEye.h"      // 娜迦眼 (禁用跟踪)
//#include "doeEye.h"        // 卡通鹿眼 (禁用跟踪)

// 显示硬件设置 (屏幕类型和连接) -------------------
#define TFT_COUNT 1        // 屏幕数量 (1 或 2)
#define TFT1_CS 5          // TFT 1 片选引脚
#define TFT2_CS -1         // TFT 2 片选引脚
#define TFT_1_ROT 1        // TFT 1 旋转
#define TFT_2_ROT 1        // TFT 2 旋转
#define EYE_1_XPOSITION  0         // 显示器上眼睛1图像的x偏移
#define EYE_2_XPOSITION  320 - 128 // 显示器上眼睛2图像的x偏移

#define DISPLAY_BACKLIGHT  -1 // 背光控制引脚 (-1 表示无)
#define BACKLIGHT_MAX    255

// 眼睛列表 ----------------------------------------------------------------
#define NUM_EYES 1 // 要显示的眼睛数量 (1 或 2)

#define BLINK_PIN   -1 // 手动眨眼按钮引脚 (两只眼睛)
#define LH_WINK_PIN -1 // 左眨眼引脚 (设为 -1 表示无引脚)
#define RH_WINK_PIN -1 // 右眨眼引脚 (设为 -1 表示无引脚)

// 此表包含每只眼睛一行。表必须以此名称存在，并包含一行或多行。
// 每行包含三个项目：相应TFT/OLED显示器的SELECT线的引脚号，
// 该眼睛的"眨眼"按钮的引脚号（如果不使用则为-1），屏幕旋转值（0-3）
// 和该眼睛的x位置偏移。

// 输入设置 (用于控制眼睛运动) -----------------------------

// JOYSTICK_X_PIN 和 JOYSTICK_Y_PIN 指定模拟输入引脚，用于手动控制眼睛
// 使用模拟摇杆。如果设置为-1或未定义，眼睛将自行移动。
// IRIS_PIN 指定光敏电阻的模拟输入引脚，使瞳孔对光线做出反应
// (或用于手动控制的电位器)。如果设置为-1或未定义，瞳孔将自行变化。

//#define JOYSTICK_X_PIN ADC1_CHANNEL_0 // 眼睛水平位置的模拟引脚 (否则自动)
//#define JOYSTICK_Y_PIN ADC1_CHANNEL_1 // 眼睛垂直位置的模拟引脚 (")
//#define JOYSTICK_X_FLIP   // 如果定义，反转摇杆X轴
//#define JOYSTICK_Y_FLIP   // 如果定义，反转摇杆Y轴
#define TRACKING            // 如果定义，眼睑跟踪瞳孔
#define AUTOBLINK           // 如果定义，眼睛也会自主眨眼

//#define LIGHT_PIN      -1 // 光线传感器引脚
#define LIGHT_CURVE  0.33 // 光线传感器调整曲线
#define LIGHT_MIN       0 // 光线传感器的最小有用读数
#define LIGHT_MAX    1023 // 传感器的最大有用读数

#define IRIS_SMOOTH         // 如果启用，过滤来自IRIS_PIN的输入
#define IRIS_MIN       90   // 最亮光线下的虹膜大小 (0-1023)
#define IRIS_MAX      130   // 最暗光线下的虹膜大小 (0-1023)

// 眨眼状态定义
#define NOBLINK 0       // 当前没有眨眼
#define ENBLINK 1       // 眼睑当前正在闭合
#define DEBLINK 2       // 眼睑当前正在打开

typedef struct {
  int8_t  select;
  int8_t  wink;
  uint8_t rotation;
  int16_t xposition;
} eyeInfo_t;

#if (NUM_EYES == 2)
  eyeInfo_t eyeInfo[] = {
    { TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION }, // 左眼片选和眨眼引脚，旋转和偏移
    { TFT2_CS, RH_WINK_PIN, TFT_2_ROT, EYE_2_XPOSITION }, // 右眼片选和眨眼引脚，旋转和偏移
  };
#else
  eyeInfo_t eyeInfo[] = {
    { TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION }, // 眼睛片选和眨眼引脚，旋转和偏移
  };
#endif

// 输入设置 (用于控制眼睛运动) -----------------------------

// JOYSTICK_X_PIN 和 JOYSTICK_Y_PIN 指定模拟输入引脚，用于手动控制眼睛
// 使用模拟摇杆。如果设置为-1或未定义，眼睛将自行移动。
// IRIS_PIN 指定光敏电阻的模