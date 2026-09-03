#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

#include "controller.h"
#include "TFT.h"
#include "SPI.h"
#include "menu.h"
#include "dino.h"
#include "bmo.h"

typedef enum
{
    BMO_SCREEN,
    MENU,
    TETRIS,
    SNAKE,
    PONG,
    DINO
} GAMESTATE;

int main(void)
{
    SPI_Init();
    TFT_INIT();
    controller_init();

    TFT_DrawBMOFace();

    GAMESTATE state = BMO_SCREEN;
    uint16_t blink_counter = 0;

    while (true)
    {
        if (state == BMO_SCREEN)
        {
            _delay_ms(20);
            blink_counter++;

            // Blink every ~2.5 seconds (125 ticks * 20ms = 2500ms)
            if (blink_counter == 125)
            {
                TFT_BMO_SetEyes(1); // Close eyes
            }
            else if (blink_counter >= 133) // Closed for 8 * 20ms = 160ms
            {
                TFT_BMO_SetEyes(0); // Re-open eyes
                blink_counter = 0;
            }

            if (get_key(START))
            {
                TFT_FillScreen(TFT_BLACK);
                menu();
                state = MENU;
                blink_counter = 0;
            }
        }
        else if (state == MENU)
        {
            if (get_key(DOWN))
            {
                menu_down();
                menu();
            }

            if (get_key(UP))
            {
                menu_up();
                menu();
            }

            if (get_key(BACK))
            {
                // Return to BMO face screen
                TFT_DrawBMOFace();
                state = BMO_SCREEN;
                blink_counter = 0;
            }

            if (get_key(START))
            {
                TFT_FillScreen(TFT_BLACK);

                if (menu_get_selected() == 0)
                {
                    state = TETRIS;
                    TFT_DrawString(20, 60, "TETRIS", TFT_RED, TFT_BLACK, 1);
                }
                else if (menu_get_selected() == 1)
                {
                    state = SNAKE;
                    TFT_DrawString(20, 60, "snake", TFT_RED, TFT_BLACK, 1);
                }
                else if (menu_get_selected() == 2)
                {
                    state = PONG;
                    TFT_DrawString(20, 60, "pong", TFT_RED, TFT_BLACK, 1);
                }
                else if (menu_get_selected() == 3)
                {
                    state = DINO;
                    play_dino();
                    TFT_FillScreen(TFT_BLACK);
                    menu();
                    state = MENU;
                }
            }
        }
        else
        {
            if (get_key(BACK))
            {
                state = MENU;
                TFT_FillScreen(TFT_BLACK);
                menu();
            }
        }
    }
}