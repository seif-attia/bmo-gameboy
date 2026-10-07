#include "flappybird.h"
#include "controller.h"
#include "TFT.h"
#include "timer.h"

#define bird_radius 4
#define bird_color 0xf708

void play_flappy_bird()
{
    uint8_t bird_x = TFT_WIDTH / 2 - bird_radius;
    uint8_t bird_y = TFT_HEIGHT / 2 - bird_radius;
    uint8_t bird_y_prev = bird_y;

    uint8_t end_game = false;

    // Physics variables
    int16_t velocity = 0;
    uint8_t gravity_counter = 0;

    uint32_t frame_timer = millis();

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
        uint32_t current_time = millis();

        if (get_key(BACK))
        {
            end_game = true;
            break;
        }

        // Run updates every 16ms (60fps)
        if (current_time - frame_timer >= 16)
        {
            frame_timer = current_time;

            if (get_key(UP))
            {
                velocity = -4;
            }

            gravity_counter++;
            if (gravity_counter >= 3)
            {
                gravity_counter = 0;
                if (velocity < 2)
                {
                    velocity++;
                }
            }

            bird_y += velocity;

            if (bird_y < bird_radius)
                bird_y = bird_radius;
            if (bird_y > TFT_HEIGHT - bird_radius)
                bird_y = TFT_HEIGHT - bird_radius;

            if (bird_y != bird_y_prev)
            {
                TFT_FillCircle(bird_x, bird_y_prev, bird_radius, TFT_BLACK);
                TFT_FillCircle(bird_x, bird_y, bird_radius, bird_color);
                bird_y_prev = bird_y;
            }
        }
    }
}