#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

#include "controller.h"
#include "TFT.h"
#include "SPI.h"
#include "menu.h"
#include "dino.h"
#include "bmo.h"
#include "timer.h"
#include "serial.h"

typedef enum
{
    BMO_SCREEN,
    MENU
} GAMESTATE;

int main(void)
{
    SPI_Init();
    TFT_INIT();
    controller_init();
    timer_init();
    serial_init(9600);

    TFT_DrawBMOFace();

    GAMESTATE state = BMO_SCREEN;

    uint32_t last_blink = 0;
    uint8_t eyes_closed = 0;

    while (true)
    {
        timer_update();

        uint32_t current_time = millis();

        if (state == BMO_SCREEN)
        {
            /*  Set eye >> 1: Eyes close
                Set eye >> 0: Eyes open

                Eyes start opened
            */
            if (!eyes_closed && (current_time - last_blink >= 2500))
            {
                TFT_BMO_SetEyes(1); // Close eyes
                eyes_closed = true;
                last_blink = current_time;
            }
            else if (eyes_closed && (current_time - last_blink >= 200))
            {
                TFT_BMO_SetEyes(0); // Open eyes
                eyes_closed = false;
                last_blink = current_time;
            }

            if (current_time - last_blink >= 0)

                if (get_key(START))
                {

                    TFT_FillScreen(TFT_BLACK);
                    menu();
                    state = MENU;
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
                last_blink = millis();
                TFT_DrawBMOFace();
                state = BMO_SCREEN;
            }

            if (get_key(START))
            {
                TFT_FillScreen(TFT_BLACK);

                if (menu_get_selected() == 0)
                {
                    TFT_DrawString(20, 60, "TETRIS", TFT_RED, TFT_BLACK, 1);
                    /* ADD game entry point later and uncomment the lines below */
                    // TFT_FillScreen(TFT_BLACK);
                    // menu();
                }
                else if (menu_get_selected() == 1)
                {
                    TFT_DrawString(20, 60, "snake", TFT_RED, TFT_BLACK, 1);
                    /* ADD game entry point later and uncomment the lines below */
                    // TFT_FillScreen(TFT_BLACK);
                    // menu();
                }
                else if (menu_get_selected() == 2)
                {
                    TFT_DrawString(20, 60, "pong", TFT_RED, TFT_BLACK, 1);
                    /* ADD game entry point later and uncomment the lines below */
                    // TFT_FillScreen(TFT_BLACK);
                    // menu();
                }
                else if (menu_get_selected() == 3)
                {
                    play_dino();
                    TFT_FillScreen(TFT_BLACK);
                    menu();
                }
            }
        }
    }
}