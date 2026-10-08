#include "flappybird.h"
#include "controller.h"
#include "TFT.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>

#define bird_radius 4
#define bird_color 0xf708

#define PIPE_WIDTH 18
#define INITIAL_GAP 58    // Starting vertical gap
#define MIN_GAP 38        // Hardest vertical gap allowed
#define PIPE_SPACING 100  // Spacing between consecutive pipes
#define MAX_PIPES 3       // Sufficient for landscape screens
#define PIPE_COLOR 0x07E0 // RGB565 Green

typedef struct
{
    int16_t x;
    uint8_t gap_y;  // Top coordinate of the gap
    uint8_t gap_h;  // Height of the gap
    uint8_t active; // Whether the pipe is currently on screen
    uint8_t passed;
} Pipe;

Pipe pipes[MAX_PIPES];
uint16_t score = 0;
uint16_t high_score = 0;
uint8_t pipe_speed = 2;
uint8_t current_gap = INITIAL_GAP;

void spawn_pipe(uint8_t index, int16_t start_x)
{
    pipes[index].x = start_x;
    pipes[index].gap_h = current_gap;

    // Prevent gap from generating too close to screen top/bottom borders
    uint8_t min_y = 15;
    uint8_t max_y = TFT_HEIGHT - 15 - pipes[index].gap_h;
    pipes[index].gap_y = min_y + (rand() % (max_y - min_y + 1));

    pipes[index].active = 1;
    pipes[index].passed = 0;
}

void init_pipes()
{
    score = 0;
    pipe_speed = 2;
    current_gap = INITIAL_GAP;

    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        pipes[i].active = 0;
        pipes[i].passed = 0;
    }

    // Spawn the first pipe just off the right edge
    spawn_pipe(0, TFT_WIDTH);
}

void update_difficulty()
{
    // Smooth speed progression: starts at 2, increases every 5 points up to 4
    pipe_speed = 2 + (score / 5);
    if (pipe_speed > 4)
        pipe_speed = 4;

    // Shrink gap slightly every 3 points to increase precision demands
    if (INITIAL_GAP > (score / 3))
    {
        current_gap = INITIAL_GAP - (score / 3);
        if (current_gap < MIN_GAP)
            current_gap = MIN_GAP;
    }
}

void update_and_draw_pipes(uint8_t bird_x)
{
    update_difficulty();

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
            if (score > high_score)
            {
                high_score = score;
            }
        }

        // --- DRAW PIPE MOVEMENTS ---
        for (int16_t x = new_x; x < old_x; x++)
        {
            // Render new trailing edge column
            if (x >= 0 && x < TFT_WIDTH)
            {
                TFT_DrawVLine_Fast(x, 0, pipes[i].gap_y, PIPE_COLOR);
                TFT_DrawVLine_Fast(x, pipes[i].gap_y + pipes[i].gap_h, TFT_HEIGHT - (pipes[i].gap_y + pipes[i].gap_h), PIPE_COLOR);
            }

            // Clear old leading edge column
            int16_t clear_x = x + PIPE_WIDTH;
            if (clear_x >= 0 && clear_x < TFT_WIDTH)
            {
                TFT_DrawVLine_Fast(clear_x, 0, pipes[i].gap_y, TFT_BLACK);
                TFT_DrawVLine_Fast(clear_x, pipes[i].gap_y + pipes[i].gap_h, TFT_HEIGHT - (pipes[i].gap_y + pipes[i].gap_h), TFT_BLACK);
            }
        }

        // --- MULTI-PIPE SPAWN LOGIC ---
        // If this pipe moved past the spacing threshold, spawn next pipe if inactive
        if (old_x >= (TFT_WIDTH - PIPE_SPACING) && new_x < (TFT_WIDTH - PIPE_SPACING))
        {
            for (uint8_t j = 0; j < MAX_PIPES; j++)
            {
                if (!pipes[j].active)
                {
                    spawn_pipe(j, TFT_WIDTH);
                    break;
                }
            }
        }

        // Recycle pipe when completely off-screen
        if (pipes[i].x + PIPE_WIDTH < 0)
        {
            pipes[i].active = 0;
        }
    }
}

void draw_score()
{

    char score_str[8];
    sprintf(score_str, "%u", score);

    // Overwrite previous region using dark background text rendering
    TFT_DrawString(TFT_WIDTH / 2 - 8, 8, score_str, TFT_WHITE, TFT_BLACK, 2);
}

// AABB Collision with dynamic gap check
uint8_t check_pipe_collision(uint8_t bird_x, uint8_t bird_y)
{
    for (uint8_t i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active)
            continue;

        if (bird_x + bird_radius >= pipes[i].x && bird_x - bird_radius <= pipes[i].x + PIPE_WIDTH)
        {
            if ((bird_y - bird_radius < pipes[i].gap_y) ||
                (bird_y + bird_radius > pipes[i].gap_y + pipes[i].gap_h))
            {
                return 1; // Collision detected
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
        TFT_FillScreen(TFT_BLACK);

        uint8_t bird_x = TFT_WIDTH / 4;
        uint8_t bird_y = TFT_HEIGHT / 2 - bird_radius;
        uint8_t bird_y_prev = bird_y;

        uint8_t end_game = false;
        int16_t velocity = 0;
        uint8_t gravity_counter = 0;

        uint32_t frame_timer = millis();

        // 1. START SCREEN
        TFT_FillCircle_Fast(bird_x, bird_y, bird_radius, bird_color);
        TFT_DrawString(TFT_WIDTH / 4 + 7, TFT_HEIGHT - 30, "PRESS START", TFT_WHITE, TFT_BLACK, 1);

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

        TFT_DrawString(TFT_WIDTH / 4 + 7, TFT_HEIGHT - 30, "PRESS START", TFT_BLACK, TFT_BLACK, 1);
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

            if (current_time - frame_timer >= 16) // ~60 FPS
            {
                frame_timer = current_time;

                if (get_key(UP))
                {
                    velocity = -5;
                }

                // Smooth Gravity Simulation
                gravity_counter++;
                if (gravity_counter >= 2)
                {
                    gravity_counter = 0;
                    if (velocity < 3)
                    {
                        velocity++;
                    }
                }

                // Erase old bird position
                TFT_FillCircle_Fast(bird_x, bird_y_prev, bird_radius, TFT_BLACK);

                bird_y += velocity;

                // Screen boundaries & floor collision
                if (bird_y < bird_radius)
                {
                    bird_y = bird_radius;
                    velocity = 0;
                }
                if (bird_y >= TFT_HEIGHT - bird_radius)
                {
                    bird_y = TFT_HEIGHT - bird_radius;
                    end_game = true; // Hit ground
                }

                // Update pipes and score
                update_and_draw_pipes(bird_x);

                // Draw bird at new position
                TFT_FillCircle_Fast(bird_x, bird_y, bird_radius, bird_color);
                bird_y_prev = bird_y;

                draw_score();

                // Collision check
                if (check_pipe_collision(bird_x, bird_y))
                {
                    end_game = true;
                }
            }
        }

        // 3. GAME OVER SCREEN
        if (play_again)
        {
            TFT_FillScreen(TFT_BLACK);
            TFT_DrawString(TFT_WIDTH / 4 - 15, TFT_HEIGHT / 4, "GAME OVER", TFT_RED, TFT_BLACK, 2);

            char score_buf[20];
            sprintf(score_buf, "SCORE: %u", score);
            TFT_DrawString(TFT_WIDTH / 4 + 10, TFT_HEIGHT / 2 - 10, score_buf, TFT_WHITE, TFT_BLACK, 1);

            sprintf(score_buf, "BEST : %u", high_score);
            TFT_DrawString(TFT_WIDTH / 4 + 10, TFT_HEIGHT / 2 + 5, score_buf, bird_color, TFT_BLACK, 1);

            TFT_DrawString(TFT_WIDTH / 6 + 16, TFT_HEIGHT - 30, "START: REPLAY", TFT_GREEN, TFT_BLACK, 1);
            TFT_DrawString(TFT_WIDTH / 6 + 16, TFT_HEIGHT - 15, "BACK : QUIT", TFT_WHITE, TFT_BLACK, 1);

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

    TFT_FillScreen(TFT_BLACK);
}