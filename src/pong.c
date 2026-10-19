#include "pong.h"
#include "controller.h"
#include "TFT.h"
#include "timer.h"
#include "music.h"
#include "sfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Theme Colors (RGB565) - Adventure Time Theme
// Shores & River
#define COLOR_GRASS 0x5E68         // Lush vibrant grass green (RGB 92, 205, 66)
#define COLOR_GRASS_DARK 0x3CE3    // Deeper shoreline grass accent
#define COLOR_GRASS_LIGHT 0x8729   // Bright grass highlight
#define COLOR_RIVER 0x4E7F         // Bright cartoonish river blue (RGB 74, 203, 255)
#define COLOR_RIVER_DEEP 0x2CF8    // Mid-river blue depth / wave highlights
#define COLOR_RIVER_SPARKLE 0xBFFF // Light shimmering water crest
#define COLOR_RIVER_BANK 0x84B2    // Soft sandy shore edge / bank
#define COLOR_FLOWER_RED 0xF986    // Red wildflower
#define COLOR_FLOWER_YEL 0xFFE0    // Yellow wildflower
#define COLOR_STONE 0x94B2         // Little pebble gray

// Tennis Ball
#define COLOR_TENNIS_BALL 0xD228 // Vibrant electric tennis-ball neon yellow-green
#define COLOR_TENNIS_SEAM 0xFFFF // White seam detail

// Victory Phrases Colors
#define COLOR_MATHEMATICAL 0x3FE0 // Vibrant Adventure Time bright green (RGB 62, 252, 0)
#define COLOR_BACON 0xD228        // Crispy glazed bacon color (rich reddish-brown / cooked strip)
#define COLOR_BACON_FAT 0xF4E8    // Bacon fat marbling highlight

// Finn Palette (P1)
#define COLOR_FINN_HAT 0xFFFF      // Finn's iconic white bear hat
#define COLOR_FINN_HAT_SHD 0xCE79  // Hat shadow / ear definition
#define COLOR_FINN_SKIN 0xFDE8     // Finn's face skin tone
#define COLOR_FINN_SHIRT 0x5EBF    // Finn's light blue shirt
#define COLOR_FINN_BACKPACK 0x24C6 // Finn's two-tone green backpack
#define COLOR_FINN_SHORTS 0x2217   // Finn's dark denim shorts
#define COLOR_FINN_BOOTS 0x0000    // Finn's black shoes

// Jake Palette (P2)
#define COLOR_JAKE_BODY 0xFDE0   // Jake's golden mustard yellow
#define COLOR_JAKE_EAR 0xD400    // Jake's ear / dark contour
#define COLOR_JAKE_SHADOW 0xEB00 // Warm golden shade
#define COLOR_JAKE_EYE_W 0xFFFF  // Jake's large cartoon white eyes
#define COLOR_JAKE_EYE_P 0x0000  // Jake's big black pupil
#define COLOR_JAKE_NOSE 0x0000   // Jake's black nose tip

// Arena Layout
#define RIVER_LEFT 48
#define RIVER_WIDTH 64
#define RIVER_RIGHT (RIVER_LEFT + RIVER_WIDTH) // 112

#define PADDLE_W 6
#define PADDLE_H 25
#define PADDLE_SPEED 2
#define BALL_SIZE 4

#define TOP_BOUNDARY 12
#define BOTTOM_BOUNDARY 116
#define PLAY_HEIGHT (BOTTOM_BOUNDARY - TOP_BOUNDARY)

#define LEFT_PADDLE_X 3
#define RIGHT_PADDLE_X (TFT_WIDTH - 3 - PADDLE_W)

#define POINTS_TO_WIN_ROUND 5

// Overall round match scores (reset on power off)
static uint16_t p1_rounds = 0;
static uint16_t p2_rounds = 0;

// Current round point scores
static uint8_t p1_points = 0;
static uint8_t p2_points = 0;

static int16_t p1_y;
static int16_t p2_y;
static int16_t p1_old_y;
static int16_t p2_old_y;

static int16_t ball_x;
static int16_t ball_y;
static int16_t ball_old_x;
static int16_t ball_old_y;
static int8_t ball_vx;
static int8_t ball_vy;

// Helper to determine background color at any (x, y) on the playfield
static inline uint16_t get_playfield_bg(int16_t x)
{
    if (x < RIVER_LEFT)
    {
        return COLOR_GRASS;
    }
    else if (x >= RIVER_RIGHT)
    {
        return COLOR_GRASS;
    }
    else
    {
        return COLOR_RIVER;
    }
}

static void draw_arena(void)
{
    // Draw top separator line (below rounds score header)
    TFT_DrawHLine_Fast(0, TOP_BOUNDARY - 1, TFT_WIDTH, 0x8C71); // Earthy outline

    // Draw bottom separator line (above current round score footer)
    TFT_DrawHLine_Fast(0, BOTTOM_BOUNDARY, TFT_WIDTH, 0x8C71);

    // Left Grassy Shore (Finn's side: x = 0 .. RIVER_LEFT - 1)
    TFT_FillRect(0, TOP_BOUNDARY, RIVER_LEFT, PLAY_HEIGHT, COLOR_GRASS);

    // Left riverbank (double layered bank with depth)
    TFT_DrawVLine_Fast(RIVER_LEFT - 2, TOP_BOUNDARY, PLAY_HEIGHT, COLOR_GRASS_DARK);
    TFT_DrawVLine_Fast(RIVER_LEFT - 1, TOP_BOUNDARY, PLAY_HEIGHT, COLOR_RIVER_BANK);

    // Wide River running down the middle (x = RIVER_LEFT .. RIVER_RIGHT - 1)
    TFT_FillRect(RIVER_LEFT, TOP_BOUNDARY, RIVER_WIDTH, PLAY_HEIGHT, COLOR_RIVER);

    // Right riverbank (double layered bank with depth)
    TFT_DrawVLine_Fast(RIVER_RIGHT, TOP_BOUNDARY, PLAY_HEIGHT, COLOR_RIVER_BANK);
    TFT_DrawVLine_Fast(RIVER_RIGHT + 1, TOP_BOUNDARY, PLAY_HEIGHT, COLOR_GRASS_DARK);

    // Right Grassy Shore (Jake's side: x = RIVER_RIGHT + 2 .. TFT_WIDTH - 1)
    TFT_FillRect(RIVER_RIGHT + 2, TOP_BOUNDARY, TFT_WIDTH - (RIVER_RIGHT + 2), PLAY_HEIGHT, COLOR_GRASS);

    // Cartoon flowing river current, waves, ripples and bright water sparkles
    for (uint8_t y = TOP_BOUNDARY + 4; y < BOTTOM_BOUNDARY - 6; y += 11)
    {
        uint8_t wave_type = (y / 11) % 3;
        if (wave_type == 0)
        {
            // Left stream ripple
            TFT_DrawHLine_Fast(RIVER_LEFT + 8, y, 12, COLOR_RIVER_DEEP);
            TFT_DrawHLine_Fast(RIVER_LEFT + 10, y + 1, 8, COLOR_RIVER_SPARKLE);
        }
        else if (wave_type == 1)
        {
            // Center wide swirl
            TFT_DrawHLine_Fast(RIVER_LEFT + 24, y, 16, COLOR_RIVER_DEEP);
            TFT_DrawHLine_Fast(RIVER_LEFT + 28, y + 1, 10, COLOR_RIVER_SPARKLE);
            TFT_DrawPixel(RIVER_LEFT + 22, y, COLOR_RIVER_SPARKLE);
        }
        else
        {
            // Right stream ripple
            TFT_DrawHLine_Fast(RIVER_LEFT + 42, y, 14, COLOR_RIVER_DEEP);
            TFT_DrawHLine_Fast(RIVER_LEFT + 44, y + 1, 9, COLOR_RIVER_SPARKLE);
        }
    }

    // River stepping stones / lily pads
    TFT_FillRect(RIVER_LEFT + 6, TOP_BOUNDARY + 28, 4, 3, COLOR_STONE);
    TFT_DrawPixel(RIVER_LEFT + 7, TOP_BOUNDARY + 29, 0xC618); // Pebble highlight
    TFT_FillRect(RIVER_LEFT + 52, TOP_BOUNDARY + 74, 4, 3, COLOR_STONE);
    TFT_DrawPixel(RIVER_LEFT + 53, TOP_BOUNDARY + 75, 0xC618);

    // Left Shore Detail (Finn's side): grass blades, flower patches, pebbles
    // Grass tufts
    TFT_DrawVLine_Fast(16, TOP_BOUNDARY + 10, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(15, TOP_BOUNDARY + 11, COLOR_GRASS_DARK);
    TFT_DrawPixel(17, TOP_BOUNDARY + 11, COLOR_GRASS_DARK);

    TFT_DrawVLine_Fast(28, TOP_BOUNDARY + 38, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(27, TOP_BOUNDARY + 39, COLOR_GRASS_DARK);

    TFT_DrawVLine_Fast(20, TOP_BOUNDARY + 66, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(21, TOP_BOUNDARY + 67, COLOR_GRASS_DARK);

    TFT_DrawVLine_Fast(38, TOP_BOUNDARY + 85, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(37, TOP_BOUNDARY + 86, COLOR_GRASS_DARK);

    // Wildflower dots (red & yellow with bright centers)
    TFT_DrawPixel(34, TOP_BOUNDARY + 16, COLOR_FLOWER_RED);
    TFT_DrawPixel(35, TOP_BOUNDARY + 16, COLOR_FLOWER_YEL);
    TFT_DrawPixel(18, TOP_BOUNDARY + 48, COLOR_FLOWER_YEL);
    TFT_DrawPixel(30, TOP_BOUNDARY + 74, COLOR_FLOWER_RED);
    TFT_DrawPixel(14, TOP_BOUNDARY + 92, COLOR_FLOWER_YEL);

    // Right Shore Detail (Jake's side): grass patches and flower blossoms
    TFT_DrawVLine_Fast(128, TOP_BOUNDARY + 14, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(127, TOP_BOUNDARY + 15, COLOR_GRASS_DARK);
    TFT_DrawPixel(129, TOP_BOUNDARY + 15, COLOR_GRASS_DARK);

    TFT_DrawVLine_Fast(142, TOP_BOUNDARY + 42, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(143, TOP_BOUNDARY + 43, COLOR_GRASS_DARK);

    TFT_DrawVLine_Fast(130, TOP_BOUNDARY + 70, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(131, TOP_BOUNDARY + 71, COLOR_GRASS_DARK);

    TFT_DrawVLine_Fast(145, TOP_BOUNDARY + 90, 3, COLOR_GRASS_DARK);
    TFT_DrawPixel(144, TOP_BOUNDARY + 91, COLOR_GRASS_DARK);

    // Flowers
    TFT_DrawPixel(122, TOP_BOUNDARY + 24, COLOR_FLOWER_RED);
    TFT_DrawPixel(138, TOP_BOUNDARY + 30, COLOR_FLOWER_YEL);
    TFT_DrawPixel(124, TOP_BOUNDARY + 56, COLOR_FLOWER_YEL);
    TFT_DrawPixel(140, TOP_BOUNDARY + 78, COLOR_FLOWER_RED);
    TFT_DrawPixel(126, TOP_BOUNDARY + 95, COLOR_FLOWER_RED);
}

// Top header: overall rounds won by Finn and Jake
static void draw_rounds_header(void)
{
    char buf[8];

    TFT_FillRect(0, 0, TFT_WIDTH, TOP_BOUNDARY - 1, TFT_BLACK);

    // Left player rounds: FINN (cyan/white hat) followed by score
    TFT_DrawString(4, 2, "FINN:", COLOR_FINN_SHIRT, TFT_BLACK, 1);
    sprintf(buf, "%u", p1_rounds);
    TFT_DrawString(34, 2, buf, TFT_WHITE, TFT_BLACK, 1);

    // Center header title: "ROUNDS" (6 chars * 6 = 36 px -> centered at (160 - 36)/2 = 62)
    TFT_DrawString(62, 2, "ROUNDS", COLOR_TENNIS_BALL, TFT_BLACK, 1);

    // Right player rounds: JAKE (yellow) followed by score
    sprintf(buf, "%u", p2_rounds);
    TFT_DrawString(114, 2, "JAKE:", COLOR_JAKE_BODY, TFT_BLACK, 1);
    TFT_DrawString(144, 2, buf, TFT_WHITE, TFT_BLACK, 1);
}

// Bottom footer: current round points (0 to 5)
static void draw_current_round_score(void)
{
    char buf[8];

    TFT_FillRect(0, BOTTOM_BOUNDARY + 1, TFT_WIDTH, TFT_HEIGHT - (BOTTOM_BOUNDARY + 1), TFT_BLACK);

    // Left player current round points
    TFT_DrawString(4, BOTTOM_BOUNDARY + 3, "FINN:", COLOR_FINN_SHIRT, TFT_BLACK, 1);
    sprintf(buf, "%u", p1_points);
    TFT_DrawString(34, BOTTOM_BOUNDARY + 3, buf, TFT_WHITE, TFT_BLACK, 1);

    // Center indicator: "FIRST 5" (7 chars * 6 = 42 px -> centered at (160 - 42)/2 = 59)
    TFT_DrawString(59, BOTTOM_BOUNDARY + 3, "FIRST 5", TFT_WHITE, TFT_BLACK, 1);

    // Right player current round points
    TFT_DrawString(114, BOTTOM_BOUNDARY + 3, "JAKE:", COLOR_JAKE_BODY, TFT_BLACK, 1);
    sprintf(buf, "%u", p2_points);
    TFT_DrawString(144, BOTTOM_BOUNDARY + 3, buf, TFT_WHITE, TFT_BLACK, 1);
}

// Draw Finn paddle with detailed Adventure Time model:
// - White bear hat with 2 cute ears
// - Face oval with skin tone + dot eyes + smile
// - Light blue shirt with green backpack strap
// - Dark blue shorts
// - Black shoes at bottom
static void draw_finn_paddle(int16_t y)
{
    // y+0: Bear ears (two small white tips with shaded center)
    TFT_FillRect(LEFT_PADDLE_X, y, PADDLE_W, 1, COLOR_GRASS);
    TFT_DrawPixel(LEFT_PADDLE_X + 1, y, COLOR_FINN_HAT);
    TFT_DrawPixel(LEFT_PADDLE_X + 4, y, COLOR_FINN_HAT);

    // y+1 .. y+2: Bear hat dome
    TFT_FillRect(LEFT_PADDLE_X, y + 1, PADDLE_W, 2, COLOR_FINN_HAT);
    TFT_DrawPixel(LEFT_PADDLE_X + 2, y + 1, COLOR_FINN_HAT_SHD);
    TFT_DrawPixel(LEFT_PADDLE_X + 3, y + 1, COLOR_FINN_HAT_SHD);

    // y+3 .. y+6: Hat opening showing Finn's face
    // Left & right border is white hat
    for (uint8_t row = 3; row <= 6; row++)
    {
        TFT_DrawPixel(LEFT_PADDLE_X, y + row, COLOR_FINN_HAT);
        TFT_DrawPixel(LEFT_PADDLE_X + 5, y + row, COLOR_FINN_HAT);
        TFT_DrawHLine_Fast(LEFT_PADDLE_X + 1, y + row, 4, COLOR_FINN_SKIN);
    }
    // Finn's cute face features: two black eyes & mouth
    TFT_DrawPixel(LEFT_PADDLE_X + 2, y + 4, 0x0000); // Eye
    TFT_DrawPixel(LEFT_PADDLE_X + 4, y + 4, 0x0000); // Eye
    TFT_DrawPixel(LEFT_PADDLE_X + 3, y + 6, 0x9800); // Smile / mouth

    // y+7: Hat chin strap / collar
    TFT_FillRect(LEFT_PADDLE_X, y + 7, PADDLE_W, 1, COLOR_FINN_HAT);

    // y+8 .. y+16: Light blue shirt + green backpack straps
    TFT_FillRect(LEFT_PADDLE_X, y + 8, PADDLE_W, 9, COLOR_FINN_SHIRT);
    // Backpack strap on back side (leftmost column) and strap accents
    TFT_DrawVLine_Fast(LEFT_PADDLE_X, y + 9, 7, COLOR_FINN_BACKPACK);
    TFT_DrawPixel(LEFT_PADDLE_X + 1, y + 11, COLOR_FINN_BACKPACK);
    TFT_DrawPixel(LEFT_PADDLE_X + 1, y + 12, COLOR_FINN_BACKPACK);

    // y+17 .. y+22: Denim dark blue shorts
    TFT_FillRect(LEFT_PADDLE_X, y + 17, PADDLE_W, 6, COLOR_FINN_SHORTS);
    // Leg separation slit in the middle
    TFT_DrawVLine_Fast(LEFT_PADDLE_X + 2, y + 21, 2, 0x10A9);

    // y+23 .. y+24: Legs & black shoes
    TFT_DrawHLine_Fast(LEFT_PADDLE_X, y + 23, PADDLE_W, COLOR_FINN_SKIN);
    TFT_FillRect(LEFT_PADDLE_X, y + 24, PADDLE_W, 1, COLOR_FINN_BOOTS);
}

// Draw Jake paddle with detailed Adventure Time model:
// - Golden yellow round head with floppy dog ear
// - Jake's signature twin round cartoon white eyes with black pupils
// - Famous drooping jowls, smiling mouth, and cute black nose
// - Chunky bulldog body and paw curves
static void draw_jake_paddle(int16_t y)
{
    // Base body: bright mustard golden yellow
    TFT_FillRect(RIGHT_PADDLE_X, y, PADDLE_W, PADDLE_H, COLOR_JAKE_BODY);

    // y+0: Head top curve
    TFT_DrawPixel(RIGHT_PADDLE_X, y, COLOR_GRASS);
    TFT_DrawPixel(RIGHT_PADDLE_X + 5, y, COLOR_GRASS);
    TFT_DrawPixel(RIGHT_PADDLE_X + 1, y, COLOR_JAKE_SHADOW);

    // y+1 .. y+3: Jake's floppy ear on outer side (right side)
    TFT_DrawPixel(RIGHT_PADDLE_X + 5, y + 1, COLOR_JAKE_EAR);
    TFT_DrawPixel(RIGHT_PADDLE_X + 5, y + 2, COLOR_JAKE_EAR);
    TFT_DrawPixel(RIGHT_PADDLE_X + 4, y + 2, COLOR_JAKE_SHADOW);
    TFT_DrawPixel(RIGHT_PADDLE_X + 5, y + 3, COLOR_JAKE_EAR);

    // y+3 .. y+7: Jake's twin cartoon eyes
    // Left eye (looking out toward the tennis court / river): x+1 .. x+2
    TFT_FillRect(RIGHT_PADDLE_X + 1, y + 3, 2, 4, COLOR_JAKE_EYE_W);
    // Right eye: x+3 .. x+4
    TFT_FillRect(RIGHT_PADDLE_X + 3, y + 3, 2, 4, COLOR_JAKE_EYE_W);

    // Thin dark outline separating and contouring eyes
    TFT_DrawPixel(RIGHT_PADDLE_X, y + 4, COLOR_JAKE_EAR);
    TFT_DrawPixel(RIGHT_PADDLE_X + 2, y + 3, COLOR_JAKE_SHADOW);

    // Big black pupils looking left toward Finn and the ball
    TFT_DrawPixel(RIGHT_PADDLE_X + 1, y + 4, COLOR_JAKE_EYE_P);
    TFT_DrawPixel(RIGHT_PADDLE_X + 1, y + 5, COLOR_JAKE_EYE_P);
    TFT_DrawPixel(RIGHT_PADDLE_X + 3, y + 4, COLOR_JAKE_EYE_P);
    TFT_DrawPixel(RIGHT_PADDLE_X + 3, y + 5, COLOR_JAKE_EYE_P);

    // White gleam in pupils
    TFT_DrawPixel(RIGHT_PADDLE_X + 1, y + 4, COLOR_JAKE_EYE_W);
    TFT_DrawPixel(RIGHT_PADDLE_X + 3, y + 4, COLOR_JAKE_EYE_W);

    // y+8: Upper muzzle bridge
    TFT_DrawHLine_Fast(RIGHT_PADDLE_X + 1, y + 8, 4, COLOR_JAKE_BODY);

    // y+9 .. y+10: Little black nose right in center of muzzle
    TFT_FillRect(RIGHT_PADDLE_X + 2, y + 9, 2, 2, COLOR_JAKE_NOSE);

    // y+11 .. y+13: Jake's famous big drooping mustache-like jowls
    TFT_DrawHLine_Fast(RIGHT_PADDLE_X + 1, y + 11, 4, COLOR_JAKE_SHADOW);
    TFT_DrawPixel(RIGHT_PADDLE_X, y + 12, COLOR_JAKE_EAR);
    TFT_DrawPixel(RIGHT_PADDLE_X + 5, y + 12, COLOR_JAKE_EAR);
    // Mouth smile line under jowls
    TFT_DrawHLine_Fast(RIGHT_PADDLE_X + 1, y + 13, 4, COLOR_JAKE_EAR);

    // y+14 .. y+21: Yellow torso with little arm contour
    TFT_DrawPixel(RIGHT_PADDLE_X + 4, y + 17, COLOR_JAKE_SHADOW);
    TFT_DrawPixel(RIGHT_PADDLE_X + 3, y + 18, COLOR_JAKE_SHADOW);
    TFT_DrawPixel(RIGHT_PADDLE_X + 4, y + 19, COLOR_JAKE_SHADOW);

    // y+22 .. y+24: Rounded bulldog feet / paws
    TFT_DrawHLine_Fast(RIGHT_PADDLE_X + 1, y + 23, 4, COLOR_JAKE_SHADOW);
    TFT_DrawPixel(RIGHT_PADDLE_X + 2, y + 24, COLOR_JAKE_EAR);
    TFT_DrawPixel(RIGHT_PADDLE_X + 4, y + 24, COLOR_JAKE_EAR);
    TFT_DrawPixel(RIGHT_PADDLE_X, y + 24, COLOR_GRASS);
    TFT_DrawPixel(RIGHT_PADDLE_X + 5, y + 24, COLOR_GRASS);
}

// Draw the tennis ball with iconic optic yellow color and white curved seam
static void draw_tennis_ball(int16_t x, int16_t y)
{
    TFT_FillRect(x, y, BALL_SIZE, BALL_SIZE, COLOR_TENNIS_BALL);
    // Curved white tennis ball seam highlight
    TFT_DrawPixel(x + 1, y, COLOR_TENNIS_SEAM);
    TFT_DrawPixel(x + 2, y + 1, COLOR_TENNIS_SEAM);
    TFT_DrawPixel(x + 2, y + 2, COLOR_TENNIS_SEAM);
    TFT_DrawPixel(x + 1, y + 3, COLOR_TENNIS_SEAM);
}

// Erase a rectangle by redrawing the underlying shore/river terrain
static void clear_playfield_rect(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
    for (int16_t col = x; col < x + w; col++)
    {
        if (col < 0 || col >= TFT_WIDTH)
            continue;

        uint16_t color = get_playfield_bg(col);

        // Bank boundaries
        if (col == RIVER_LEFT - 1 || col == RIVER_RIGHT)
        {
            color = COLOR_RIVER_BANK;
        }

        TFT_DrawVLine_Fast(col, y, h, color);
    }
}

// Reset ball with randomized initial trajectory
static void reset_ball(uint8_t serve_to_player)
{
    // Clear old ball before repositioning
    clear_playfield_rect(ball_old_x, ball_old_y, BALL_SIZE, BALL_SIZE);

    ball_x = (TFT_WIDTH - BALL_SIZE) / 2;
    ball_y = TOP_BOUNDARY + (PLAY_HEIGHT - BALL_SIZE) / 2;
    ball_old_x = ball_x;
    ball_old_y = ball_y;

    if (serve_to_player == 0)
    {
        // Random horizontal direction (serve to Finn or Jake)
        ball_vx = (rand() % 2 == 0) ? 2 : -2;
    }
    else
    {
        ball_vx = (serve_to_player == 1) ? -2 : 2;
    }

    // Random vertical direction and speed (-2, -1, 1, 2)
    int8_t v_choices[4] = {-2, -1, 1, 2};
    ball_vy = v_choices[rand() % 4];

    draw_tennis_ball(ball_x, ball_y);
}

static void render_paddles(void)
{
    // Erase old left paddle parts and redraw Finn
    if (p1_y != p1_old_y)
    {
        if (p1_y > p1_old_y)
        {
            TFT_FillRect(LEFT_PADDLE_X, p1_old_y, PADDLE_W, p1_y - p1_old_y, COLOR_GRASS);
        }
        else
        {
            TFT_FillRect(LEFT_PADDLE_X, p1_y + PADDLE_H, PADDLE_W, (p1_old_y + PADDLE_H) - (p1_y + PADDLE_H), COLOR_GRASS);
        }
        draw_finn_paddle(p1_y);
        p1_old_y = p1_y;
    }

    // Erase old right paddle parts and redraw Jake
    if (p2_y != p2_old_y)
    {
        if (p2_y > p2_old_y)
        {
            TFT_FillRect(RIGHT_PADDLE_X, p2_old_y, PADDLE_W, p2_y - p2_old_y, COLOR_GRASS);
        }
        else
        {
            TFT_FillRect(RIGHT_PADDLE_X, p2_y + PADDLE_H, PADDLE_W, (p2_old_y + PADDLE_H) - (p2_y + PADDLE_H), COLOR_GRASS);
        }
        draw_jake_paddle(p2_y);
        p2_old_y = p2_y;
    }
}

static void render_ball(void)
{
    if (ball_x != ball_old_x || ball_y != ball_old_y)
    {
        clear_playfield_rect(ball_old_x, ball_old_y, BALL_SIZE, BALL_SIZE);
        draw_tennis_ball(ball_x, ball_y);
        ball_old_x = ball_x;
        ball_old_y = ball_y;
    }
}

void play_pong(void)
{
    // Seed PRNG with current timer value so random trajectories vary
    srand((unsigned int)millis());

    bool exit_game = false;

    while (!exit_game)
    {
        // Initialize new round (first to 5 points)
        p1_points = 0;
        p2_points = 0;

        p1_y = TOP_BOUNDARY + (PLAY_HEIGHT - PADDLE_H) / 2;
        p2_y = TOP_BOUNDARY + (PLAY_HEIGHT - PADDLE_H) / 2;
        p1_old_y = p1_y;
        p2_old_y = p2_y;

        reset_ball(0);

        TFT_FillScreen(TFT_BLACK);
        draw_arena();
        draw_rounds_header();
        draw_current_round_score();

        // Initial render of paddles and ball
        draw_finn_paddle(p1_y);
        draw_jake_paddle(p2_y);
        draw_tennis_ball(ball_x, ball_y);

        // Prompt to start the round
        // "PRESS START" is 11 chars * 6 = 66 px -> centered at (160 - 66)/2 = 47
        TFT_DrawString(47, 58, "PRESS START", COLOR_TENNIS_BALL, COLOR_RIVER, 1);
        while (true)
        {
            timer_update();
            update_music();

            if (get_key(BACK))
            {
                exit_game = true;
                break;
            }
            if (get_key(START))
            {
                break;
            }
        }

        if (exit_game)
        {
            break;
        }

        // Clear start message and redraw river arena
        draw_arena();
        draw_finn_paddle(p1_y);
        draw_jake_paddle(p2_y);
        draw_tennis_ball(ball_x, ball_y);

        uint32_t last_frame_time = millis();

        // Round gameplay loop
        while (!exit_game)
        {
            timer_update();
            update_music();

            if (get_key(BACK))
            {
                exit_game = true;
                break;
            }

            uint32_t now = millis();
            if (now - last_frame_time >= 25) // Fixed ~40 Hz tick rate
            {
                last_frame_time = now;

                // Paddle controls
                // Left player: UP and DOWN keys
                if (!GET_BIT(CONTROLLER_PIN, UP))
                {
                    if (p1_y > TOP_BOUNDARY)
                    {
                        p1_y -= PADDLE_SPEED;
                        if (p1_y < TOP_BOUNDARY)
                            p1_y = TOP_BOUNDARY;
                    }
                }
                if (!GET_BIT(CONTROLLER_PIN, DOWN))
                {
                    if (p1_y + PADDLE_H < BOTTOM_BOUNDARY)
                    {
                        p1_y += PADDLE_SPEED;
                        if (p1_y + PADDLE_H > BOTTOM_BOUNDARY)
                            p1_y = BOTTOM_BOUNDARY - PADDLE_H;
                    }
                }

                // Right player: LEFT (up) and RIGHT (down) keys
                if (!GET_BIT(CONTROLLER_PIN, LEFT))
                {
                    if (p2_y > TOP_BOUNDARY)
                    {
                        p2_y -= PADDLE_SPEED;
                        if (p2_y < TOP_BOUNDARY)
                            p2_y = TOP_BOUNDARY;
                    }
                }
                if (!GET_BIT(CONTROLLER_PIN, RIGHT))
                {
                    if (p2_y + PADDLE_H < BOTTOM_BOUNDARY)
                    {
                        p2_y += PADDLE_SPEED;
                        if (p2_y + PADDLE_H > BOTTOM_BOUNDARY)
                            p2_y = BOTTOM_BOUNDARY - PADDLE_H;
                    }
                }

                // Move ball
                ball_x += ball_vx;
                ball_y += ball_vy;

                // Specular reflection off top and bottom boundaries
                if (ball_y <= TOP_BOUNDARY)
                {
                    ball_y = TOP_BOUNDARY;
                    ball_vy = -ball_vy;
                    play_sfx(sfx_pong_wall);
                }
                else if (ball_y + BALL_SIZE >= BOTTOM_BOUNDARY)
                {
                    ball_y = BOTTOM_BOUNDARY - BALL_SIZE;
                    ball_vy = -ball_vy;
                    play_sfx(sfx_pong_wall);
                }

                // Specular reflection off left paddle
                if (ball_vx < 0 && ball_x <= LEFT_PADDLE_X + PADDLE_W)
                {
                    if (ball_y + BALL_SIZE >= p1_y && ball_y <= p1_y + PADDLE_H)
                    {
                        ball_x = LEFT_PADDLE_X + PADDLE_W;
                        ball_vx = -ball_vx;
                        play_sfx(sfx_pong_hit);
                    }
                }

                // Specular reflection off right paddle
                if (ball_vx > 0 && ball_x + BALL_SIZE >= RIGHT_PADDLE_X)
                {
                    if (ball_y + BALL_SIZE >= p2_y && ball_y <= p2_y + PADDLE_H)
                    {
                        ball_x = RIGHT_PADDLE_X - BALL_SIZE;
                        ball_vx = -ball_vx;
                        play_sfx(sfx_pong_hit);
                    }
                }

                // Point scored check
                if (ball_x < 0)
                {
                    // Player 2 scores a point
                    p2_points++;
                    play_sfx(sfx_pong_score);
                    draw_current_round_score();

                    if (p2_points >= POINTS_TO_WIN_ROUND)
                    {
                        p2_rounds++;
                        draw_rounds_header();
                        break; // End of this round
                    }
                    reset_ball(1);
                }
                else if (ball_x + BALL_SIZE > TFT_WIDTH)
                {
                    // Player 1 scores a point
                    p1_points++;
                    play_sfx(sfx_pong_score);
                    draw_current_round_score();

                    if (p1_points >= POINTS_TO_WIN_ROUND)
                    {
                        p1_rounds++;
                        draw_rounds_header();
                        break; // End of this round
                    }
                    reset_ball(2);
                }

                render_paddles();
                render_ball();
            }
        }

        // Show round winner banner if round ended with a win
        if (!exit_game && (p1_points >= POINTS_TO_WIN_ROUND || p2_points >= POINTS_TO_WIN_ROUND))
        {
            play_sfx(sfx_tictactoe_win);

            // Banner box centered: 120 px wide -> (160 - 120)/2 = 20, 32 px high -> y=48..80
            TFT_FillRect(20, 48, 120, 32, TFT_BLACK);
            TFT_DrawHLine_Fast(20, 48, 120, TFT_WHITE);
            TFT_DrawHLine_Fast(20, 79, 120, TFT_WHITE);
            TFT_DrawVLine_Fast(20, 48, 32, TFT_WHITE);
            TFT_DrawVLine_Fast(139, 48, 32, TFT_WHITE);

            // Center aligned within the 160px screen:
            // "FINN WINS!" (10 chars * 6 = 60 px) -> x = (160 - 60)/2 = 50
            // "JAKE WINS!" (10 chars * 6 = 60 px) -> x = (160 - 60)/2 = 50
            // "MATHEMATICAL!" (13 chars * 6 = 78 px) -> x = (160 - 78)/2 = 41
            // "BACON!" (6 chars * 6 = 36 px) -> x = (160 - 36)/2 = 62
            if (p1_points >= POINTS_TO_WIN_ROUND)
            {
                TFT_DrawString(50, 55, "FINN WINS!", COLOR_FINN_SHIRT, TFT_BLACK, 1);
                TFT_DrawString(41, 67, "MATHEMATICAL!", COLOR_MATHEMATICAL, TFT_BLACK, 1);
            }
            else
            {
                TFT_DrawString(50, 55, "JAKE WINS!", COLOR_JAKE_BODY, TFT_BLACK, 1);
                TFT_DrawString(62, 67, "BACON!", COLOR_BACON, TFT_BLACK, 1);
            }

            // Wait for START to start next round or BACK to return to menu
            while (true)
            {
                timer_update();
                update_music();

                if (get_key(BACK))
                {
                    exit_game = true;
                    break;
                }
                if (get_key(START))
                {
                    break; // Proceed to next round
                }
            }
        }
    }
    stop_song();
}
