#include <avr/io.h>

#include "menu.h"
#include "TFT.h"

static uint8_t selected_game = 0;

void menu(void)
{

    if (selected_game == 0)
        TFT_DrawString(5, 35, ">>", MENU_TEXT, MENU_BACKGROUND, 1);
    else
        TFT_DrawString(5, 35, "  ", MENU_TEXT, MENU_BACKGROUND, 1);

    TFT_DrawString(20, 35, "TIC-TAC-TOE", MENU_TEXT, MENU_BACKGROUND, 1);

    if (selected_game == 1)
        TFT_DrawString(5, 55, ">>", MENU_TEXT, MENU_BACKGROUND, 1);
    else
        TFT_DrawString(5, 55, "  ", MENU_TEXT, MENU_BACKGROUND, 1);

    TFT_DrawString(20, 55, "snake", MENU_TEXT, MENU_BACKGROUND, 1);

    if (selected_game == 2)
        TFT_DrawString(5, 75, ">>", MENU_TEXT, MENU_BACKGROUND, 1);
    else
        TFT_DrawString(5, 75, "  ", MENU_TEXT, MENU_BACKGROUND, 1);

    TFT_DrawString(20, 75, "FlappyBird", MENU_TEXT, MENU_BACKGROUND, 1);

    if (selected_game == 3)
        TFT_DrawString(5, 95, ">>", MENU_TEXT, MENU_BACKGROUND, 1);
    else
        TFT_DrawString(5, 95, "  ", MENU_TEXT, MENU_BACKGROUND, 1);

    TFT_DrawString(20, 95, "DINO", MENU_TEXT, MENU_BACKGROUND, 1);

    if (selected_game == 4)
        TFT_DrawString(5, 115, ">>", MENU_TEXT, MENU_BACKGROUND, 1);
    else
        TFT_DrawString(5, 115, "  ", MENU_TEXT, MENU_BACKGROUND, 1);

    TFT_DrawString(20, 115, "PONG", MENU_TEXT, MENU_BACKGROUND, 1);
}

void menu_down(void)
{
    if (selected_game < 4)
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