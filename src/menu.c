#include <avr/io.h>

#include "menu.h"
#include "TFT.h"

static uint8_t selected_game = 0;

void menu(void)
{
    TFT_DrawString(20, 10, "BMO game", TFT_RED, TFT_BLACK, 1);

    if (selected_game == 0)
        TFT_DrawString(5, 35, ">>", TFT_RED, TFT_BLACK, 1);
    else
        TFT_DrawString(5, 35, "  ", TFT_RED, TFT_BLACK, 1);

    TFT_DrawString(20, 35, "TETRIS", TFT_RED, TFT_BLACK, 1);

    if (selected_game == 1)
        TFT_DrawString(5, 55, ">>", TFT_RED, TFT_BLACK, 1);
    else
        TFT_DrawString(5, 55, "  ", TFT_RED, TFT_BLACK, 1);

    TFT_DrawString(20, 55, "snake", TFT_RED, TFT_BLACK, 1);

    if (selected_game == 2)
        TFT_DrawString(5, 75, ">>", TFT_RED, TFT_BLACK, 1);
    else
        TFT_DrawString(5, 75, "  ", TFT_RED, TFT_BLACK, 1);

    TFT_DrawString(20, 75, "FlappyBird", TFT_RED, TFT_BLACK, 1);

    if (selected_game == 3)
        TFT_DrawString(5, 95, ">>", TFT_RED, TFT_BLACK, 1);
    else
        TFT_DrawString(5, 95, "  ", TFT_RED, TFT_BLACK, 1);

    TFT_DrawString(20, 95, "DINO", TFT_RED, TFT_BLACK, 1);
}

void menu_down(void)
{
    if (selected_game < 3)
    {
        selected_game++;
    }
}

void menu_up(void)
{
    if (selected_game > 0)
    {
        selected_game--;
    }
}

uint8_t menu_get_selected(void)
{
    return selected_game;
}