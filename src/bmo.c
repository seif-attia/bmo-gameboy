#include "TFT.h"
#include "bmo.h"

void TFT_DrawBMOFace(void)
{
    // 1. Fill background with light mint green
    TFT_FillScreen(BMO_BG_COLOR);

    // 2. Draw Eyes
    TFT_BMO_SetEyes(0);

    // 3. Draw BMO Mouth (pixel-accurate smiling mouth matching reference image)

    // Outer black mouth shape (creates black border & outline)
    TFT_DrawHLine_Fast(64, 69, 33, BMO_BLACK);
    TFT_DrawHLine_Fast(61, 70, 39, BMO_BLACK);
    TFT_DrawHLine_Fast(59, 71, 43, BMO_BLACK);
    TFT_DrawHLine_Fast(57, 72, 47, BMO_BLACK);
    TFT_DrawHLine_Fast(56, 73, 49, BMO_BLACK);
    TFT_DrawHLine_Fast(56, 74, 49, BMO_BLACK);
    TFT_DrawHLine_Fast(56, 75, 49, BMO_BLACK);
    TFT_DrawHLine_Fast(56, 76, 49, BMO_BLACK);
    TFT_DrawHLine_Fast(57, 77, 47, BMO_BLACK);
    TFT_DrawHLine_Fast(57, 78, 47, BMO_BLACK);
    TFT_DrawHLine_Fast(58, 79, 45, BMO_BLACK);
    TFT_DrawHLine_Fast(59, 80, 43, BMO_BLACK);
    TFT_DrawHLine_Fast(60, 81, 41, BMO_BLACK);
    TFT_DrawHLine_Fast(61, 82, 39, BMO_BLACK);
    TFT_DrawHLine_Fast(63, 83, 35, BMO_BLACK);
    TFT_DrawHLine_Fast(65, 84, 31, BMO_BLACK);
    TFT_DrawHLine_Fast(68, 85, 25, BMO_BLACK);
    TFT_DrawHLine_Fast(72, 86, 17, BMO_BLACK);

    // Dark green interior cavity (inset 2px inside black border)
    TFT_DrawHLine_Fast(63, 71, 35, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(60, 72, 41, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(58, 73, 45, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(58, 74, 45, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(58, 75, 45, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(58, 76, 45, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(59, 77, 43, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(59, 78, 43, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(60, 79, 41, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(61, 80, 39, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(62, 81, 37, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(63, 82, 35, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(65, 83, 31, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(67, 84, 27, BMO_MOUTH_DARK);
    TFT_DrawHLine_Fast(70, 85, 21, BMO_MOUTH_DARK);

    // White teeth band across upper interior
    TFT_DrawHLine_Fast(63, 71, 35, BMO_WHITE);
    TFT_DrawHLine_Fast(61, 72, 39, BMO_WHITE);
    TFT_DrawHLine_Fast(59, 73, 43, BMO_WHITE);
    TFT_DrawHLine_Fast(59, 74, 43, BMO_WHITE);

    // Tongue (bright green curve at bottom of mouth)
    TFT_DrawHLine_Fast(70, 81, 21, BMO_TONGUE);
    TFT_DrawHLine_Fast(67, 82, 27, BMO_TONGUE);
    TFT_DrawHLine_Fast(66, 83, 29, BMO_TONGUE);
    TFT_DrawHLine_Fast(68, 84, 25, BMO_TONGUE);
    TFT_DrawHLine_Fast(71, 85, 19, BMO_TONGUE);
}

void TFT_BMO_SetEyes(uint8_t closed)
{

    if (closed)
    {
        // Erase eye circles with background color
        TFT_FillCircle_Fast(45, 52, 6, BMO_BG_COLOR);
        TFT_FillCircle_Fast(115, 52, 6, BMO_BG_COLOR);

        // Draw closed eye lines (blinking expression)
        TFT_DrawHLine_Fast(39, 52, 13, BMO_BLACK);
        TFT_DrawHLine_Fast(39, 53, 13, BMO_BLACK);
        TFT_DrawHLine_Fast(109, 52, 13, BMO_BLACK);
        TFT_DrawHLine_Fast(109, 53, 13, BMO_BLACK);
    }
    else
    {
        // Erase closed eye lines with background color
        TFT_FillRect(38, 50, 15, 5, BMO_BG_COLOR);
        TFT_FillRect(108, 50, 15, 5, BMO_BG_COLOR);

        // Draw solid black circle eyes
        TFT_FillCircle_Fast(45, 52, 6, BMO_BLACK);
        TFT_FillCircle_Fast(115, 52, 6, BMO_BLACK);
    }
}