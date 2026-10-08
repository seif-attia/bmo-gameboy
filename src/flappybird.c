#include "flappybird.h"
#include "controller.h"
#include "TFT.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>

#define bird_radius 4
#define bird_color 0xf708

#define PIPE_WIDTH 16
#define PIPE_GAP 55       // Vertical gap height for the bird to fly through
#define MAX_PIPES 4       // 2 pipes are usually enough to tile standard TFT screen widths
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
uint8_t pipe_speed = 2;

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
}

void update_and_draw_pipes(uint8_t bird_x)
{
    // Increase speed when score > 10
    if (score / 10 >= 1)
        pipe_speed = 4;

    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active)
            continue;

        int16_t old_x = pipes[i].x;
        pipes[i].x -= pipe_speed;
        int16_t new_x = pipes[i].x;

        // --- SCORE CHECK ---
        if (!pipes[i].passed && (pipes[i].x + PIPE_WIDTH < bird_x))
        {
            pipes[i].passed = 1;
            score++;
        }

        // --- DRAWING PIPES ---
        for (int16_t x = new_x; x < old_x; x++)
        {
            if (x >= 0 && x < TFT_WIDTH)
            {
                TFT_DrawVLine_Fast(x, 0, pipes[i].gap_y, PIPE_COLOR);
                TFT_DrawVLine_Fast(x, pipes[i].gap_y + PIPE_GAP, TFT_HEIGHT - (pipes[i].gap_y + PIPE_GAP), PIPE_COLOR);
            }

            int16_t clear_x = x + PIPE_WIDTH;
            if (clear_x >= 0 && clear_x < TFT_WIDTH)
            {
                TFT_DrawVLine_Fast(clear_x, 0, pipes[i].gap_y, TFT_BLACK);
                TFT_DrawVLine_Fast(clear_x, pipes[i].gap_y + PIPE_GAP, TFT_HEIGHT - (pipes[i].gap_y + PIPE_GAP), TFT_BLACK);
            }
        }

        // Recycle pipe off-screen
        if (pipes[i].x + PIPE_WIDTH < 0)
        {
            pipes[i].x = TFT_WIDTH;
            pipes[i].gap_y = 20 + (rand() % (TFT_HEIGHT - 40 - PIPE_GAP));
            pipes[i].passed = 0;
        }
    }
}

void draw_score()
{
    char score_str[8];
    sprintf(score_str, "%u", score);

    TFT_DrawString(TFT_WIDTH / 2 - 4, 10, score_str, TFT_WHITE, TFT_BLACK, 2);
}

// AABB Collision
uint8_t check_pipe_collision(uint8_t bird_x, uint8_t bird_y)
{
    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active)
            continue;

        if (bird_x + bird_radius >= pipes[i].x && bird_x - bird_radius <= pipes[i].x + PIPE_WIDTH)
        {
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
    uint8_t play_again = true;

    while (play_again)
    {
        // Clear screen at start of each game session
        TFT_FillScreen(TFT_BLACK);

        uint8_t bird_x = TFT_WIDTH / 4;
        uint8_t bird_y = TFT_HEIGHT / 2 - bird_radius;
        uint8_t bird_y_prev = bird_y;

        uint8_t end_game = false;

        // Physics variables
        int16_t velocity = 0;
        uint8_t gravity_counter = 0;

        uint32_t frame_timer = millis();

        // 1. START SCREEN
        TFT_FillCircle_Fast(bird_x, bird_y, bird_radius, bird_color);
        TFT_DrawString(TFT_WIDTH / 4 + 5, TFT_HEIGHT - 20, "PRESS START", TFT_WHITE, TFT_BLACK, 1);

        while (!get_key(START))
        {
            if (get_key(BACK))
            {
                play_again = false;
                break;
            }
        }

        if (!play_again)
            break;

        // Erase start prompt & setup pipes
        TFT_DrawString(TFT_WIDTH / 4 + 5, TFT_HEIGHT - 20, "PRESS START", TFT_BLACK, TFT_BLACK, 1);
        init_pipes();

        // 2. GAMEPLAY LOOP
        while (!end_game)
        {
            uint32_t current_time = millis();

            if (get_key(BACK))
            {
                end_game = true;
                play_again = false;
                break;
            }

            if (current_time - frame_timer >= 16)
            {
                frame_timer = current_time;

                if (get_key(UP))
                {
                    velocity = -5;
                }

                gravity_counter++;
                if (gravity_counter >= 2)
                {
                    gravity_counter = 0;
                    if (velocity < 3)
                    {
                        velocity++;
                    }
                }

                // Remove old position
                TFT_FillCircle_Fast(bird_x, bird_y_prev, bird_radius, TFT_BLACK);

                bird_y += velocity;

                // Screen boundaries & floor collision
                if (bird_y < bird_radius)
                {
                    bird_y = bird_radius;
                }
                if (bird_y > TFT_HEIGHT - bird_radius)
                {
                    bird_y = TFT_HEIGHT - bird_radius;
                    end_game = true; // Hit ground
                }

                if (bird_y != bird_y_prev)
                {
                    TFT_FillCircle_Fast(bird_x, bird_y, bird_radius, bird_color);
                    bird_y_prev = bird_y;
                }

                // Update pipes and score
                update_and_draw_pipes(bird_x);

                TFT_FillCircle_Fast(bird_x, bird_y, bird_radius, bird_color);
                bird_y_prev = bird_y;

                draw_score();

                // Check pipe collision
                if (check_pipe_collision(bird_x, bird_y))
                {
                    end_game = true;
                }
            }
        }

        // 3. GAME OVER SCREEN (Only if user didn't quit with BACK)
        if (play_again)
        {
            TFT_FillScreen(TFT_BLACK);
            TFT_DrawString(TFT_WIDTH / 4 - 15, TFT_HEIGHT / 3, "GAME OVER", TFT_RED, TFT_BLACK, 2);

            char final_score[16];
            sprintf(final_score, "SCORE: %u", score);
            TFT_DrawString(TFT_WIDTH / 4 + 17, TFT_HEIGHT / 2, final_score, TFT_WHITE, TFT_BLACK, 1);

            TFT_DrawString(TFT_WIDTH / 6 + 14, TFT_HEIGHT - 30, "START: REPLAY", TFT_GREEN, TFT_BLACK, 1);
            TFT_DrawString(TFT_WIDTH / 6 + 14, TFT_HEIGHT - 15, "BACK: QUIT", TFT_WHITE, TFT_BLACK, 1);

            while (1)
            {
                if (get_key(START))
                {
                    play_again = true;
                    break;
                }
                if (get_key(BACK))
                {
                    play_again = false;
                    break;
                }
            }
        }
    }

    // Clean exit
    TFT_FillScreen(TFT_BLACK);
}