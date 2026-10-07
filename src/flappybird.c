#include "flappybird.h"
#include "controller.h"
#include "TFT.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>

#define bird_radius 4
#define bird_color 0xf708

#define PIPE_WIDTH 16
#define PIPE_GAP 45       // Vertical gap height for the bird to fly through
#define PIPE_SPEED 2      // Horizontal pixels moved per frame
#define MAX_PIPES 2       // 2 pipes are usually enough to tile standard TFT screen widths
#define PIPE_COLOR 0x07E0 // RGB565 Green

typedef struct
{
    int16_t x;
    uint8_t gap_y;  // Top coordinate of the gap
    uint8_t active; // Whether the pipe is currently on screen
    uint8_t passed;
} Pipe;

Pipe pipes[MAX_PIPES];
uint16_t score = 0;
uint8_t score_changed = false;

void init_pipes()
{
    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        pipes[i].active = 0;
        pipes[i].passed = 0;
    }

    pipes[0].x = TFT_WIDTH;
    pipes[0].gap_y = 20 + (rand() % (TFT_HEIGHT - 40 - PIPE_GAP));
    pipes[0].active = 1;
    pipes[0].passed = 0;

    score = 0;
    score_changed = true; // Force initial draw of score 0
}

void update_and_draw_pipes(uint8_t bird_x)
{
    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active)
            continue;

        int16_t old_x = pipes[i].x;
        pipes[i].x -= PIPE_SPEED;
        int16_t new_x = pipes[i].x;

        // --- SCORE CHECK ---
        // Increment score when the trailing edge of the pipe passes bird_x
        if (!pipes[i].passed && (pipes[i].x + PIPE_WIDTH < bird_x))
        {
            pipes[i].passed = 1;
            score++;
            score_changed = true;
        }

        // --- DRAWING PIPES ---
        for (int16_t x = new_x; x < old_x; x++)
        {
            if (x >= 0 && x < TFT_WIDTH)
            {
                TFT_DrawVLine(x, 0, pipes[i].gap_y, PIPE_COLOR);
                TFT_DrawVLine(x, pipes[i].gap_y + PIPE_GAP, TFT_HEIGHT - (pipes[i].gap_y + PIPE_GAP), PIPE_COLOR);
            }

            int16_t clear_x = x + PIPE_WIDTH;
            if (clear_x >= 0 && clear_x < TFT_WIDTH)
            {
                TFT_DrawVLine(clear_x, 0, TFT_HEIGHT, TFT_BLACK);
            }
        }

        // Recycle pipe off-screen
        if (pipes[i].x + PIPE_WIDTH < 0)
        {
            pipes[i].x = TFT_WIDTH;
            pipes[i].gap_y = 20 + (rand() % (TFT_HEIGHT - 40 - PIPE_GAP));
            pipes[i].passed = 0; // Reset passed state for the recycled pipe
        }
    }
}

void draw_score()
{
    if (score_changed)
    {
        char score_str[8];
        sprintf(score_str, "%u", score);

        // Render text with TFT_BLACK as background to overwrite old digits directly
        TFT_DrawString(TFT_WIDTH / 2 - 4, 10, score_str, TFT_WHITE, TFT_BLACK, 2);
        score_changed = false;
    }
}

uint8_t check_pipe_collision(uint8_t bird_x, uint8_t bird_y)
{
    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active)
            continue;

        // Check horizontal overlap with the pipe
        if (bird_x + bird_radius >= pipes[i].x && bird_x - bird_radius <= pipes[i].x + PIPE_WIDTH)
        {
            // Check vertical overlap (hitting the top or bottom pipe)
            if ((bird_y - bird_radius < pipes[i].gap_y) ||
                (bird_y + bird_radius > pipes[i].gap_y + PIPE_GAP))
            {
                return 1; // Collision detected!
            }
        }
    }
    return 0;
}

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

    init_pipes();

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

            // --- PIPES & SCORE ---
            update_and_draw_pipes(bird_x);
            draw_score();

            // if (check_pipe_collision(bird_x, bird_y))
            // {
            //     end_game = true;
            // }
        }
    }
}