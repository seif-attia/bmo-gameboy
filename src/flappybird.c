#include "flappybird.h"
#include "controller.h"
#include "TFT.h"
#include "SPI.h"
#include "BIT_MATH.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "music.h"
#include "sfx.h"

// Hardware control helpers
#define TFT_CS_LOW() CLR_BIT(PORTB, TFT_CS)
#define TFT_CS_HIGH() SET_BIT(PORTB, TFT_CS)
#define TFT_DC_HIGH() SET_BIT(PORTB, TFT_DC)

// Gameplay Dimensions
#define MARCY_W 14
#define MARCY_H 14

#define SPIKE_WIDTH 20
#define INITIAL_GAP 64
#define INITIAL_SPACING 100
#define MIN_GAP 46
#define MAX_SPIKES 3
#define PLAY_CEILING_Y 12

// Lightweight unsigned integer to ASCII conversion (no sprintf)
static void utoa16(uint16_t val, char *buf)
{
    char *p = buf;
    if (val == 0)
    {
        *p++ = '0';
        *p = '\0';
        return;
    }
    while (val > 0)
    {
        *p++ = '0' + (val % 10);
        val /= 10;
    }
    char *end = p - 1;
    while (buf < end)
    {
        char tmp = *buf;
        *buf++ = *end;
        *end-- = tmp;
    }
    *p = '\0';
}

// Fixed-point scaling factor: 1 pixel = 16 sub-pixel units
#define FP_SHIFT 4
#define FP_SCALE (1 << FP_SHIFT)

// Solid Black Background Palette (RGB565)
#define COLOR_BG 0x0000 // Pure solid black

// Rectangular Stone Spike / Pipe Palette
#define COLOR_SPIKE_DARK 0x2945  // Deep charcoal shadow flank of stone pipe
#define COLOR_SPIKE_BODY 0x5ACB  // Mid stone gray body
#define COLOR_SPIKE_LIGHT 0x9CE7 // Highlighted left flank of stone pipe
#define COLOR_SPIKE_POINT 0xD69A // Highlight stone cap / rim

// UI / Vampire Accents
#define COLOR_VAMP_RED 0xF800      // Bright blood red
#define COLOR_VAMP_DARK_RED 0x6800 // Deep blood crimson
#define COLOR_VAMP_PALE 0xDEFB     // Marceline pale skin accent

static const uint16_t marcy_palette[16] PROGMEM = {
    COLOR_BG, 0x0000, 0x18C5, 0xCE79,
    0xF800, 0x3186, 0x528A, 0x2A54,
    0x118C, 0x59A4, 0xD800, 0xFFFF,
    0xBDF7, 0x0841, 0xFFFF, 0x8800};

static const uint8_t marceline_float_sprite[98] PROGMEM = {
    0x00, 0x01, 0x11, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x11, 0x21, 0x11, 0x10, 0x00, 0x00,
    0x01, 0x13, 0x33, 0x14, 0x11, 0x00, 0x00,
    0x01, 0x13, 0x3E, 0x31, 0x11, 0x00, 0x00,
    0x11, 0x11, 0x34, 0x41, 0x11, 0x00, 0x00,
    0x12, 0x11, 0x56, 0x51, 0x1C, 0xC0, 0x00,
    0x11, 0x11, 0x55, 0x5A, 0xAC, 0x00, 0x00,
    0x01, 0x11, 0x77, 0xAB, 0xBA, 0x00, 0x00,
    0x00, 0x11, 0x78, 0xAA, 0xFA, 0x00, 0x00,
    0x00, 0x01, 0x17, 0x7A, 0xAA, 0x00, 0x00,
    0x00, 0x00, 0x17, 0x89, 0x90, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x99, 0x90, 0x00, 0x00,
    0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00};

static const uint8_t marceline_rise_sprite[98] PROGMEM = {
    0x01, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x21, 0x11, 0x11, 0x00, 0x00, 0x00,
    0x11, 0x13, 0x33, 0x14, 0x10, 0x00, 0x00,
    0x12, 0x13, 0x3E, 0x31, 0x10, 0x00, 0x00,
    0x11, 0x11, 0x34, 0x41, 0x10, 0x00, 0x00,
    0x11, 0x15, 0x65, 0x1C, 0xC0, 0x00, 0x00,
    0x01, 0x15, 0x55, 0xAA, 0xCC, 0x00, 0x00,
    0x00, 0x17, 0x7A, 0xBB, 0xAA, 0x00, 0x00,
    0x00, 0x17, 0x8A, 0xAA, 0xFA, 0x00, 0x00,
    0x00, 0x11, 0x77, 0x8A, 0xAA, 0x00, 0x00,
    0x00, 0x01, 0x17, 0x99, 0x90, 0x00, 0x00,
    0x00, 0x00, 0x19, 0x90, 0x00, 0x00, 0x00,
    0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef struct
{
    int32_t x_fp;   // Sub-pixel X coordinate (x * 16)
    uint8_t gap_y;  // Top coordinate of the gap (>= PLAY_CEILING_Y)
    uint8_t gap_h;  // Height of the gap
    uint8_t active; // Whether the pillar is currently on screen
    uint8_t passed;
} SpikePillar;

static SpikePillar spikes[MAX_SPIKES];
static uint16_t score = 0;
static uint16_t high_score = 0;

static uint16_t spike_speed = 16;
static uint8_t current_gap = INITIAL_GAP;
static uint8_t spike_spacing = INITIAL_SPACING;

static void spawn_spike_pillar(uint8_t index, int16_t start_x)
{
    spikes[index].x_fp = (int32_t)start_x * FP_SCALE;
    spikes[index].gap_h = current_gap;

    uint8_t min_y = PLAY_CEILING_Y + 4;
    uint8_t max_y = TFT_HEIGHT - 16 - spikes[index].gap_h;
    if (max_y < min_y)
        max_y = min_y;

    spikes[index].gap_y = min_y + (rand() % (max_y - min_y + 1));
    spikes[index].active = 1;
    spikes[index].passed = 0;
}

static void init_spikes(void)
{
    score = 0;
    spike_speed = 16;
    current_gap = INITIAL_GAP;
    spike_spacing = INITIAL_SPACING;

    for (uint8_t i = 0; i < MAX_SPIKES; i++)
    {
        spikes[i].active = 0;
        spikes[i].passed = 0;
    }

    spawn_spike_pillar(0, TFT_WIDTH);
}

static void update_difficulty(void)
{
    spike_speed = 16 + (score / 4);
    if (spike_speed > 32)
        spike_speed = 32;

    int16_t temp_gap = INITIAL_GAP - (score / 3);
    if (temp_gap < MIN_GAP)
        current_gap = MIN_GAP;
    else
        current_gap = (uint8_t)temp_gap;

    int16_t temp_spacing = INITIAL_SPACING - (score / 3);
    if (temp_spacing < 75)
        spike_spacing = 75;
    else
        spike_spacing = (uint8_t)temp_spacing;
}

// Fast rectangular pipe renderer: paints ONLY solid stone and caps (AIR GAP IS NEVER TOUCHED)
static void draw_pipe(int16_t x, uint8_t gap_y, uint8_t gap_h)
{
    if (x + SPIKE_WIDTH <= 0 || x >= TFT_WIDTH)
        return;

    int16_t top_h = (int16_t)gap_y - PLAY_CEILING_Y;
    int16_t bot_y = (int16_t)gap_y + gap_h;
    int16_t bot_h = TFT_HEIGHT - bot_y;

    // 1. Top stone pipe
    if (top_h > 0)
    {
        // Left highlight flank
        TFT_FillRect(x, PLAY_CEILING_Y, 1, top_h, COLOR_SPIKE_LIGHT);
        // Stone body
        if (SPIKE_WIDTH > 2)
        {
            TFT_FillRect(x + 1, PLAY_CEILING_Y, SPIKE_WIDTH - 2, top_h, COLOR_SPIKE_BODY);
        }
        // Right shadow flank
        TFT_FillRect(x + SPIKE_WIDTH - 1, PLAY_CEILING_Y, 1, top_h, COLOR_SPIKE_DARK);

        // Stone Cap / Rim at opening (2px tall, 1px wider on each side)
        if (top_h >= 2)
        {
            TFT_FillRect(x - 1, gap_y - 2, SPIKE_WIDTH + 2, 2, COLOR_SPIKE_POINT);
            TFT_FillRect(x, gap_y - 1, SPIKE_WIDTH, 1, COLOR_SPIKE_LIGHT);
        }
    }

    // 2. Bottom stone pipe
    if (bot_h > 0)
    {
        // Left highlight flank
        TFT_FillRect(x, bot_y, 1, bot_h, COLOR_SPIKE_LIGHT);
        // Stone body
        if (SPIKE_WIDTH > 2)
        {
            TFT_FillRect(x + 1, bot_y, SPIKE_WIDTH - 2, bot_h, COLOR_SPIKE_BODY);
        }
        // Right shadow flank
        TFT_FillRect(x + SPIKE_WIDTH - 1, bot_y, 1, bot_h, COLOR_SPIKE_DARK);

        // Stone Cap / Rim at opening (2px tall, 1px wider on each side)
        if (bot_h >= 2)
        {
            TFT_FillRect(x - 1, bot_y, SPIKE_WIDTH + 2, 2, COLOR_SPIKE_POINT);
            TFT_FillRect(x, bot_y, SPIKE_WIDTH, 1, COLOR_SPIKE_LIGHT);
        }
    }
}

static void update_and_draw_spikes(uint8_t marcy_x)
{
    update_difficulty();

    for (uint8_t i = 0; i < MAX_SPIKES; i++)
    {
        if (!spikes[i].active)
            continue;

        int16_t old_x = (int16_t)(spikes[i].x_fp >> FP_SHIFT);
        spikes[i].x_fp -= spike_speed;
        int16_t new_x = (int16_t)(spikes[i].x_fp >> FP_SHIFT);

        if (!spikes[i].passed && (new_x + SPIKE_WIDTH < marcy_x))
        {
            spikes[i].passed = 1;
            score++;
            play_sfx(sfx_flappy_score);
            if (score > high_score)
            {
                high_score = score;
            }
        }

        // Only redraw when pipe moves across a pixel boundary
        if (new_x != old_x)
        {
            int16_t dx = old_x - new_x;
            int16_t clear_x = new_x + SPIKE_WIDTH;
            int16_t clear_w = dx + 1; // +1 to also clear the 1px extended cap rim

            int16_t top_h = (int16_t)spikes[i].gap_y - PLAY_CEILING_Y;
            int16_t bot_y = (int16_t)spikes[i].gap_y + spikes[i].gap_h;
            int16_t bot_h = TFT_HEIGHT - bot_y;

            // Clear trailing edge on the right (ONLY solid pipe regions, AIR GAP IS NEVER TOUCHED)
            if (top_h > 0)
            {
                TFT_FillRect(clear_x, PLAY_CEILING_Y, clear_w, top_h, COLOR_BG);
            }
            if (bot_h > 0)
            {
                TFT_FillRect(clear_x, bot_y, clear_w, bot_h, COLOR_BG);
            }

            // Draw pipe at its new position
            draw_pipe(new_x, spikes[i].gap_y, spikes[i].gap_h);
        }

        // Spawn next pillar when current pillar passes spacing threshold
        if (old_x >= (TFT_WIDTH - spike_spacing) && new_x < (TFT_WIDTH - spike_spacing))
        {
            for (uint8_t j = 0; j < MAX_SPIKES; j++)
            {
                if (!spikes[j].active)
                {
                    spawn_spike_pillar(j, TFT_WIDTH);
                    break;
                }
            }
        }

        // Deactivate when completely off screen to the left
        if (new_x + SPIKE_WIDTH < -2)
        {
            spikes[i].active = 0;
        }
    }
}

static void draw_marceline(int16_t px, int16_t py, bool rising)
{
    if (px >= TFT_WIDTH || py >= TFT_HEIGHT || (px + MARCY_W) <= 0 || (py + MARCY_H) <= 0)
        return;

    const uint8_t *sprite = rising ? marceline_rise_sprite : marceline_float_sprite;

    TFT_SetAddrWindow((uint8_t)px, (uint8_t)py, (uint8_t)(px + MARCY_W - 1), (uint8_t)(py + MARCY_H - 1));
    TFT_CS_LOW();
    TFT_DC_HIGH();

    uint8_t byte_idx = 0;
    for (uint8_t y = 0; y < MARCY_H; y++)
    {
        for (uint8_t x = 0; x < MARCY_W; x += 2)
        {
            uint8_t b = pgm_read_byte(&sprite[byte_idx++]);
            uint8_t p1 = b >> 4;
            uint8_t p2 = b & 0x0F;

            TFT_spiWrite16(pgm_read_word(&marcy_palette[p1]));
            TFT_spiWrite16(pgm_read_word(&marcy_palette[p2]));
        }
    }
    TFT_CS_HIGH();
}

static void reset_score_hud(void)
{
    // Draw top HUD ceiling border
    TFT_DrawHLine_Fast(0, PLAY_CEILING_Y - 1, TFT_WIDTH, COLOR_VAMP_DARK_RED);

    TFT_DrawString(4, 2, "SCORE:", COLOR_VAMP_PALE, COLOR_BG, 1);
    char score_str[6];
    utoa16(score, score_str);
    TFT_DrawString(44, 2, score_str, COLOR_VAMP_RED, COLOR_BG, 1);

    TFT_DrawString(96, 2, "BEST:", COLOR_VAMP_PALE, COLOR_BG, 1);
    char hs_str[6];
    utoa16(high_score, hs_str);
    TFT_DrawString(128, 2, hs_str, COLOR_SPIKE_POINT, COLOR_BG, 1);
}

static void draw_score(void)
{
    static uint16_t last_drawn_score = 0xFFFF;
    if (score != last_drawn_score)
    {
        char score_str[6];
        utoa16(score, score_str);
        TFT_FillRect(44, 2, 24, 8, COLOR_BG);
        TFT_DrawString(44, 2, score_str, COLOR_VAMP_RED, COLOR_BG, 1);
        last_drawn_score = score;
    }
}

static uint8_t check_spike_collision(uint8_t marcy_x, int16_t marcy_y)
{
    // Bottom floor collision
    if (marcy_y >= TFT_HEIGHT - MARCY_H - 1)
    {
        return 1;
    }

    for (uint8_t i = 0; i < MAX_SPIKES; i++)
    {
        if (!spikes[i].active)
            continue;

        int16_t spike_x = (int16_t)(spikes[i].x_fp >> FP_SHIFT);

        // Horizontal overlap with forgiving hitbox (2px inset on both sides)
        if ((marcy_x + MARCY_W - 2) >= spike_x && (marcy_x + 2) <= (spike_x + SPIKE_WIDTH))
        {
            // Vertical collision: hit top stone pipe
            if (marcy_y < (int16_t)spikes[i].gap_y)
            {
                return 1;
            }

            // Vertical collision: hit bottom stone pipe
            if ((marcy_y + MARCY_H - 1) > (int16_t)(spikes[i].gap_y + spikes[i].gap_h))
            {
                return 1;
            }
        }
    }
    return 0;
}

static void draw_string_centered(uint8_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size)
{
    uint8_t len = 0;
    const char *p = str;
    while (*p++)
        len++;
    uint8_t str_width = len * 6 * size;
    uint8_t x = (str_width >= TFT_WIDTH) ? 0 : (TFT_WIDTH - str_width) / 2;
    TFT_DrawString(x, y, str, color, bg_color, size);
}

void play_flappy_bird(void)
{
    uint8_t play_again = true;

    while (play_again)
    {
        update_music();

        // Solid Black Screen Initializer
        TFT_FillScreen(COLOR_BG);

        uint8_t marcy_x = 26;
        int16_t marcy_y = TFT_HEIGHT / 2 - (MARCY_H / 2);
        int16_t marcy_y_prev = marcy_y;

        uint8_t end_game = false;
        int32_t marcy_y_fp = (int32_t)marcy_y << FP_SHIFT;
        int16_t velocity_fp = 0;
        uint32_t frame_timer = millis();
        bool jump_requested = false;

        draw_string_centered(20, "MARCELINE'S ESCAPE", COLOR_VAMP_RED, COLOR_BG, 1);
        draw_marceline(marcy_x, marcy_y, false);
        draw_string_centered(TFT_HEIGHT - 32, "PRESS START", COLOR_SPIKE_POINT, COLOR_BG, 1);
        draw_string_centered(TFT_HEIGHT - 18, "UP: Flap   BACK: Exit", COLOR_VAMP_PALE, COLOR_BG, 1);

        while (!get_key(START))
        {
            update_music();
            timer_update();
            if (get_key(BACK))
            {
                play_again = false;
                break;
            }
        }

        if (!play_again)
            break;

        // Clear screen to solid black for game start
        TFT_FillScreen(COLOR_BG);
        init_spikes();
        reset_score_hud();
        draw_marceline(marcy_x, marcy_y, false);
        marcy_y_prev = marcy_y;
        frame_timer = millis();

        while (!end_game)
        {
            update_music();
            timer_update();

            if (get_key(BACK))
            {
                end_game = true;
                play_again = false;
                break;
            }

            // Continuous jump polling (buffers input so fast taps are never lost)
            if (get_key(UP))
            {
                jump_requested = true;
            }

            uint32_t current_time = millis();
            if (current_time - frame_timer >= 16) // Steady 60 FPS tick
            {
                frame_timer += 16;
                if (current_time - frame_timer > 32)
                {
                    frame_timer = current_time;
                }

                // Physics update
                if (jump_requested)
                {
                    velocity_fp = -40; // Immediate upward impulse (-2.5 px/frame)
                    jump_requested = false;
                }
                else
                {
                    velocity_fp += 3; // Smooth gravitational acceleration
                    if (velocity_fp > 48)
                        velocity_fp = 48; // Terminal fall velocity (3.0 px/frame)
                }

                marcy_y_fp += velocity_fp;
                marcy_y = marcy_y_fp >> FP_SHIFT;

                // Ceiling clamp (doesn't kill, smooth bonk)
                if (marcy_y < PLAY_CEILING_Y)
                {
                    marcy_y = PLAY_CEILING_Y;
                    marcy_y_fp = (int32_t)PLAY_CEILING_Y << FP_SHIFT;
                    if (velocity_fp < 0)
                        velocity_fp = 0;
                }
                else if (marcy_y >= TFT_HEIGHT - MARCY_H - 1)
                {
                    marcy_y = TFT_HEIGHT - MARCY_H - 1;
                    play_sfx(sfx_gameover_1);
                    end_game = true;
                }

                // 1. Update and draw pipes (AIR GAP IS NEVER TOUCHED)
                update_and_draw_spikes(marcy_x);

                // 2. Erase only the exposed boundary slivers instead of the whole bounding box (eliminates flicker)
                if (marcy_y != marcy_y_prev)
                {
                    if (marcy_y > marcy_y_prev)
                    {
                        // Moving DOWN: clear the sliver left exposed above
                        int16_t dy = marcy_y - marcy_y_prev;
                        if (dy >= MARCY_H)
                            TFT_FillRect(marcy_x, marcy_y_prev, MARCY_W, MARCY_H, COLOR_BG);
                        else
                            TFT_FillRect(marcy_x, marcy_y_prev, MARCY_W, dy, COLOR_BG);
                    }
                    else
                    {
                        // Moving UP: clear the sliver left exposed below
                        int16_t dy = marcy_y_prev - marcy_y;
                        if (dy >= MARCY_H)
                            TFT_FillRect(marcy_x, marcy_y_prev, MARCY_W, MARCY_H, COLOR_BG);
                        else
                            TFT_FillRect(marcy_x, marcy_y + MARCY_H, MARCY_W, dy, COLOR_BG);
                    }
                }

                // 3. Immediately draw Marceline at new position
                draw_marceline(marcy_x, marcy_y, velocity_fp < 0);
                marcy_y_prev = marcy_y;

                // 4. Update HUD score
                draw_score();

                // 5. Collision detection
                if (check_spike_collision(marcy_x, marcy_y))
                {
                    play_sfx(sfx_gameover_1);
                    end_game = true;
                }
            }
        }

        if (play_again)
        {
            play_sfx(sfx_gameover_1);
            TFT_FillScreen(COLOR_BG);

            // Modal dialog frame
            TFT_FillRect(12, 15, 136, 98, COLOR_BG);
            TFT_DrawHLine_Fast(12, 15, 136, COLOR_SPIKE_POINT);
            TFT_DrawHLine_Fast(12, 112, 136, COLOR_SPIKE_POINT);
            TFT_DrawVLine_Fast(12, 15, 98, COLOR_SPIKE_POINT);
            TFT_DrawVLine_Fast(147, 15, 98, COLOR_SPIKE_POINT);

            TFT_DrawHLine_Fast(14, 17, 132, COLOR_VAMP_RED);
            TFT_DrawHLine_Fast(14, 110, 132, COLOR_VAMP_RED);
            TFT_DrawVLine_Fast(14, 17, 94, COLOR_VAMP_RED);
            TFT_DrawVLine_Fast(145, 17, 94, COLOR_VAMP_RED);

            TFT_FillRect(15, 18, 130, 20, 0x18C3);
            draw_string_centered(24, "TRAPPED IN THE CAVE!", COLOR_VAMP_RED, 0x18C3, 1);

            static char score_buf[16];
            const char score_label[] = "SCORE: ";
            memcpy(score_buf, score_label, sizeof(score_label) - 1);
            utoa16(score, score_buf + (sizeof(score_label) - 1));
            draw_string_centered(46, score_buf, COLOR_SPIKE_POINT, COLOR_BG, 1);

            const char best_label[] = "BEST : ";
            memcpy(score_buf, best_label, sizeof(best_label) - 1);
            utoa16(high_score, score_buf + (sizeof(best_label) - 1));
            draw_string_centered(58, score_buf, COLOR_VAMP_PALE, COLOR_BG, 1);

            TFT_FillRect(22, 74, 116, 16, 0x1B64);
            TFT_DrawHLine_Fast(22, 74, 116, 0x3FE0);
            TFT_DrawHLine_Fast(22, 89, 116, 0x3FE0);
            TFT_DrawVLine_Fast(22, 74, 16, 0x3FE0);
            TFT_DrawVLine_Fast(137, 74, 16, 0x3FE0);
            draw_string_centered(78, "START: Retry Escape", 0xFFFF, 0x1B64, 1);

            TFT_FillRect(22, 93, 116, 16, COLOR_VAMP_DARK_RED);
            TFT_DrawHLine_Fast(22, 93, 116, COLOR_VAMP_RED);
            TFT_DrawHLine_Fast(22, 108, 116, COLOR_VAMP_RED);
            TFT_DrawVLine_Fast(22, 93, 16, COLOR_VAMP_RED);
            TFT_DrawVLine_Fast(137, 93, 16, COLOR_VAMP_RED);
            draw_string_centered(97, "BACK:  Main Menu", 0xFFFF, COLOR_VAMP_DARK_RED, 1);

            while (true)
            {
                update_music();
                timer_update();
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
    stop_song();
}
