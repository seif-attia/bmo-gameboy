#pragma once
#include <stdint.h>

#define BMO_BG_COLOR     0xD736 // Light mint green
#define BMO_BLACK        0x0000 // Black border & eyes
#define BMO_WHITE        0xFFFF // White teeth
#define BMO_MOUTH_DARK   0x0280 // Dark green mouth interior
#define BMO_TONGUE       0x4EEA // Bright mint green tongue

void TFT_DrawBMOFace(void);
void TFT_BMO_SetEyes(uint8_t closed);