#include "Snake.h"
#include "TFT.h"
#include "controller.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "music.h"
#define GRID_SIZE 5
#define BOARD_WIDTH (TFT_WIDTH / GRID_SIZE)
#define BOARD_HEIGHT (TFT_HEIGHT / GRID_SIZE)
#define MAX_SNAKE_LENGTH 60

typedef enum
{
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef struct
{
    int8_t x;
    int8_t y;
} Point;

static Point snake[MAX_SNAKE_LENGTH];
static uint8_t snake_length;
static Direction current_dir;
static Direction next_dir;
static Point food;
uint16_t score;
uint32_t move_interval = 250;

static void spawn_food()
{
    bool on_snake;
    do
    {
        on_snake = false;

        uint8_t min_y = 3;
        food.x = rand() % BOARD_WIDTH;
        food.y = min_y + (rand() % (BOARD_HEIGHT - min_y));
        if (food.y < 3 && food.x < 15)
        {
            on_snake = true;
            continue;
        }

        for (uint8_t i = 0; i < snake_length; i++)
        {
            if (snake[i].x == food.x && snake[i].y == food.y)
            {
                on_snake = true;
                break;
            }
        }
    } while (on_snake);
}

static void snake_init()
{
    snake_length = 3;
    current_dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;
    score = 0;
    move_interval = 250;

    snake[0].x = 10;
    snake[0].y = 10;
    snake[1].x = 9;
    snake[1].y = 10;
    snake[2].x = 8;
    snake[2].y = 10;

    spawn_food();
}

static void draw_block(int8_t x, int8_t y, uint16_t color)
{
    TFT_FillRect(x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE - 1, GRID_SIZE - 1, color);
}

void play_snake(void)
{
    uint8_t again = true;
    while(again){
    TFT_FillScreen(TFT_BLACK);
    snake_init();

    uint32_t last_move_time = millis();
    // const uint32_t move_interval = 120;
    char s_b[6];
    uint16_t last_s = 0;

    TFT_DrawString(5, 2, "Score:", TFT_RED, TFT_BLACK, 1);
    TFT_DrawString(45, 2, "0", TFT_RED, TFT_BLACK, 1);

    TFT_DrawHLine_Fast(0, 12, 160, TFT_RED);

    uint8_t end = false;
        while (!end)
    {
        timer_update();
        update_music();
        uint32_t current_time = millis();

        if (get_key(UP) && current_dir != DIR_DOWN)
        {
            next_dir = DIR_UP;
        }
        else if (get_key(DOWN) && current_dir != DIR_UP)
        {
            next_dir = DIR_DOWN;
        }
        else if (get_key(LEFT) && current_dir != DIR_RIGHT)
        {
            next_dir = DIR_LEFT;
        }
        else if (get_key(RIGHT) && current_dir != DIR_LEFT)
        {
            next_dir = DIR_RIGHT;
        }

        if (get_key(BACK))
        {
            end = true;
            again = false;
            break;
        }

        if (current_time - last_move_time >= move_interval)
        {
            last_move_time = current_time;
            current_dir = next_dir;

            draw_block(snake[snake_length - 1].x, snake[snake_length - 1].y, TFT_BLACK);

            for (int i = snake_length - 1; i > 0; i--)
            {
                snake[i] = snake[i - 1];
            }

            if (current_dir == DIR_UP)
                snake[0].y--;
            else if (current_dir == DIR_DOWN)
                snake[0].y++;
            else if (current_dir == DIR_LEFT)
                snake[0].x--;
            else if (current_dir == DIR_RIGHT)
                snake[0].x++;

            if (snake[0].x < 0 || snake[0].x >= BOARD_WIDTH ||
                snake[0].y < 3 || snake[0].y >= BOARD_HEIGHT)
            {
                end = true;
                break;
            }

            bool self_collision = false;
            for (int i = 1; i < snake_length; i++)
            {
                if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
                {
                    self_collision = true;
                    break;
                }
            }
            if (self_collision){end = true;break;}
                

            if (snake[0].x == food.x && snake[0].y == food.y)
            {
                if (snake_length < MAX_SNAKE_LENGTH)
                {
                    snake_length++;
                }
                score += 10;
                if (move_interval > 100)
                {
                    move_interval -= 2;
                }

                spawn_food();
            }
            if (score != last_s)
            {
                last_s = score;
                itoa(score, s_b, 10);
                TFT_DrawString(45, 2, "    ", TFT_RED, TFT_BLACK, 1);
                TFT_DrawString(45, 2, s_b, TFT_RED, TFT_BLACK, 1);
            }

            draw_block(food.x, food.y, TFT_RED);
            for (uint8_t i = 0; i < snake_length; i++)
            {
                draw_block(snake[i].x, snake[i].y, TFT_WHITE);
            }
        }
    }
    if(again){
    TFT_FillScreen(TFT_BLACK);
    char text[15];
    strcpy_P(text, PSTR("GAME OVER"));
    TFT_DrawString(25, 32, text, TFT_RED, TFT_BLACK, 2);
    sprintf(s_b, "Score: %u", score);
    TFT_DrawString(50, 54, s_b, TFT_WHITE, TFT_BLACK, 1);
    TFT_DrawString(60, 69, "START", TFT_GREEN, TFT_BLACK, 1);
    TFT_DrawString(60, 84, "BACK", TFT_WHITE, TFT_BLACK, 1);


    while (true)
    {
        update_music();
        timer_update();
        if (get_key(BACK))
        {
            again = false;
            break;
        }
        if (get_key(START))
        {
            again = true;
            break;
            
        }
    }
}
}
    stop_song();
}