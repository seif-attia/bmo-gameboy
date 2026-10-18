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
#include "flappybird.h"
#include "Snake.h"
#include "tictactoe.h"
#include "pong.h"
#include "stdio.h"
#include "adc.h"
#include "battery_status.h"
#include "music.h"
#include "BIT_MATH.h"
#include "tone.h"
#include <avr/pgmspace.h>
// Music
#define MUSIC_PIN PD3

const char menuMusic[] PROGMEM = "Adventur:d=4,o=6,b=130:16p.,2a.5,8f#5,8g5,2a5,32p,d.,32p,2b.5,32p.,8d,8e,8f#,e,d,e,e,2d,32p,8d,8e,8f#,e,d,e,b5,2d,32p,b5,8a5,d,d,f#5,2e.5,32p.,8f#5,8g5,2a5,32p,d.,32p,2b.5,32p.,8d,8e,8f#,e,d,e,e,2d,32p,8f#5,8g5,8f#,e,d,e,b5,2d,32p,8g5";

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
    battery_init();
    adc_init();
    TFT_DrawBMOFace();

    tone_init();

    GAMESTATE state = BMO_SCREEN;

    uint32_t last_blink = 0;
    uint8_t eyes_closed = 0;

    // Music
    start_song(menuMusic);

    while (true)
    {
        if (is_music_playing() == false)
            start_song(menuMusic);
        timer_update();
        update_music();

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

                    TFT_FillScreen(MENU_BACKGROUND);
                    menu();
                    battery_force_redraw();
                    state = MENU;
                }
        }
        else if (state == MENU)
        {

            battery_update(current_time);

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
                    stop_song();
                    play_tic_tac_toe();
                    TFT_FillScreen(MENU_BACKGROUND);
                    menu();
                }
                else if (menu_get_selected() == 1)
                {
                    stop_song();
                    play_snake();
                    TFT_FillScreen(MENU_BACKGROUND);
                    menu();
                }
                else if (menu_get_selected() == 2)
                {
                    stop_song();
                    play_flappy_bird();
                    TFT_FillScreen(MENU_BACKGROUND);
                    menu();
                }
                else if (menu_get_selected() == 3)
                {
                    stop_song();
                    play_dino();
                    TFT_FillScreen(MENU_BACKGROUND);
                    menu();
                }
                else if (menu_get_selected() == 4)
                {
                    stop_song();
                    play_pong();
                    TFT_FillScreen(MENU_BACKGROUND);
                    menu();
                }
            }
        }
    }
}