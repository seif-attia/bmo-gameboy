#include "dino.h"
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include "TFT.h"

const uint8_t PROGMEM dino_frame0[] = {
0x00, 0x00,
    0x07, 0xFE,
    0x07, 0xDE,
    0x07, 0xFE,
    0x07, 0xFE,
    0x07, 0xFE,
    0x07, 0xC0,
    0x07, 0xF8,
    0x84, 0x70,
    0xC4, 0x20,
    0xE7, 0xF0,
    0x7F, 0xE0,
    0x3F, 0xC0,
    0x1F, 0x80,
    0x0D, 0x80,
    0x0C, 0xC0
};

const uint8_t PROGMEM cactus_sprite[] = {
    0x18, 0x18, 0x18, 0x5A, 0x5A, 0x5A, 0x5A, 0x3C, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18
};

void play_dino(void)
{
    // All game variables declared locally on the stack to save static RAM
    PlayerDino dino;
    Obstacle cactus;
    uint16_t score = 0;
    uint16_t old_score = 0xFFFF;
    uint8_t scroll_speed = 3;
    GameState current_state = GAME_STATE_START;
    GameState rendered_state = (GameState)0xFF;

    TFT_FillRect(0, 0, DINO_SCREEN_WIDTH, DINO_SCREEN_HEIGHT, 0x0000);

    while (current_state != GAME_STATE_EXIT)
    {
        //how we escape from the game 
        if (get_key(BACK))
        {
            current_state = GAME_STATE_EXIT;
            break;
        }

        switch (current_state)
        {
            //restart the game for both of two states
            case GAME_STATE_START:
            case GAME_STATE_GAME_OVER:
                if (get_key(START))
                {
                    // Reset Dino position and state
                    dino.x = DINO_X_POS;
                    dino.y = GROUND_Y - DINO_H;
                    dino.old_x = dino.x;
                    dino.old_y = dino.y;
                    dino.vy = 0;
                    dino.is_jumping = false;

                    // Reset Cactus position and state
                    cactus.x = DINO_SCREEN_WIDTH + 20;
                    cactus.y = GROUND_Y - OBSTACLE_H;
                    cactus.old_x = cactus.x;
                    cactus.old_y = cactus.y;
                    cactus.active = true;          //when it is true it means that the dino does not pass it 

                    // Reset Score & Physics Speed
                    score = 0;
                    old_score = 0xFFFF;
                    scroll_speed = 3;               //scroll speed up to 8

                    current_state = GAME_STATE_RUNNING;
                    rendered_state = GAME_STATE_RUNNING;

                    TFT_FillRect(0, 0, DINO_SCREEN_WIDTH, DINO_SCREEN_HEIGHT, 0x0000);
                }
                break;

            case GAME_STATE_RUNNING:

            //moving logic

                if (get_key(UP) && !dino.is_jumping)
                {
                    dino.vy = JUMP_IMPULSE;
                    dino.is_jumping = true;
                }
                break;

            default:
                break;
        }

        // UPDATE PHYSICS & CHECK COLLISION
        if (current_state == GAME_STATE_RUNNING)
        {
            dino.old_y = dino.y;
            cactus.old_x = cactus.x;

            // Dino Jump & Gravity
            dino.y += dino.vy;
            dino.vy += GRAVITY;

            if (dino.y >= GROUND_Y - DINO_H)
            {
                dino.y = GROUND_Y - DINO_H;
                dino.vy = 0;
                dino.is_jumping = false;
            }

            // Obstacle Movement
            cactus.x -= scroll_speed;

            // Obstacle Respawn & Score Increase
            if (cactus.x + OBSTACLE_W < 0)
            {
                cactus.x = DINO_SCREEN_WIDTH + (rand() % 40);
                score += 10;

                if (score % 50 == 0 && scroll_speed < 8)
                {
                    scroll_speed++;
                }
            }

            // Collision Check
            if (dino.x < cactus.x + OBSTACLE_W &&
                dino.x + DINO_W > cactus.x &&
                dino.y < cactus.y + OBSTACLE_H &&
                dino.y + DINO_H > cactus.y)
            {
                current_state = GAME_STATE_GAME_OVER;
            }
        }

        // 3. RENDER FRAME
        if (current_state == GAME_STATE_START)
        {
            if (rendered_state != GAME_STATE_START)
            {
                TFT_FillRect(0, 0, DINO_SCREEN_WIDTH, DINO_SCREEN_HEIGHT, 0x0000);
                TFT_DrawString(35, 40, "DINO RUNNER", 0xFFE0, 0x0000, 1);
                TFT_DrawString(30, 70, "START: Play", 0x07E0, 0x0000, 1);
                TFT_DrawString(30, 85, "BACK: Exit", 0xF800, 0x0000, 1);
                rendered_state = GAME_STATE_START;
            }
        }
        else if (current_state == GAME_STATE_GAME_OVER)
        {
            if (rendered_state != GAME_STATE_GAME_OVER)
            {
                TFT_FillRect(20, 30, 120, 70, 0x0000);
                TFT_DrawString(40, 40, "GAME OVER", 0xF800, 0x0000, 1);
                TFT_DrawString(25, 65, "START: Restart", 0x07E0, 0x0000, 1);
                TFT_DrawString(25, 80, "BACK: Exit", 0xFFFF, 0x0000, 1);
                rendered_state = GAME_STATE_GAME_OVER;
            }
        }
        else if (current_state == GAME_STATE_RUNNING)
        {
            // Clear old dino bounds
            if (dino.old_y != dino.y)
            {
                TFT_FillRect(dino.old_x, dino.old_y, DINO_W, DINO_H, 0x0000);
            }

            // Clear old cactus bounds (handles screen edge clipping cleanly)
            if (cactus.old_x != cactus.x)
            {
                TFT_FillRect(cactus.old_x, cactus.y, OBSTACLE_W, OBSTACLE_H, 0x0000);
            }

            // Draw Ground Line
            TFT_FillRect(0, GROUND_Y, DINO_SCREEN_WIDTH, 1, 0xFFFF);

            // Render Dino Sprite
            TFT_DrawBitmap1Bit_Scaled_Clipped(
                dino.x, dino.y, dino_frame0,
                DINO_W, DINO_H, 0x07E0, 0x0000, 1
            );

            // Render Cactus Sprite
            TFT_DrawBitmap1Bit_Scaled_Clipped(
                cactus.x, cactus.y, cactus_sprite,
                OBSTACLE_W, OBSTACLE_H, 0xF800, 0x0000, 1
            );

            // Drawing Score logic
            if (score != old_score)
            {
                char score_str[7];
                utoa(score, score_str, 10);

                TFT_FillRect(135, 5, 25, 8, 0x0000);
                TFT_DrawString(95, 5, "SCORE:", 0x07E0, 0x0000, 1);
                TFT_DrawString(135, 5, score_str, 0xFFFF, 0x0000, 1);

                old_score = score;
            }
        }

        _delay_ms(30);
    }
}