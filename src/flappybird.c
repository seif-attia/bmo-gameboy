#include "flappybird.h"
#include "controller.h"
#include "TFT.h"

#define bird_radius 4
#define bird_color 0xf708

void play_flappy_bird()
{
    uint8_t bird_x = TFT_WIDTH / 2 - bird_radius;
    uint8_t bird_x_prev = bird_x;
    uint8_t bird_y = TFT_HEIGHT / 2 - bird_radius;
    uint8_t bird_y_prev = bird_y;
    uint8_t end_game = false;

    while (!get_key(START))
    {
        TFT_FillCircle(bird_x, bird_y, bird_radius, bird_color);
        TFT_DrawString(TFT_WIDTH / 4 + 5, TFT_HEIGHT - 20, "PRESS START", TFT_WHITE, TFT_BLACK, 1);

        if (get_key(BACK))
        {
            end_game = true;
            break;
        }
    }

    TFT_DrawString(TFT_WIDTH / 4 + 5, TFT_HEIGHT - 20, "PRESS START", TFT_BLACK, TFT_BLACK, 1);

    while (!end_game)
    {
        if (get_key(BACK))
        {
            end_game = true;
            break;
        }

        bird_x_prev = bird_x;
        bird_y_prev = bird_y;
        TFT_FillCircle(bird_x, bird_y, bird_radius, bird_color);

        if (get_key(UP))
        {
            bird_y -= 10;
            TFT_FillCircle(bird_x_prev, bird_y_prev, bird_radius, TFT_BLACK);
            TFT_FillCircle(bird_x, bird_y, bird_radius, bird_color);
        }
    }
}