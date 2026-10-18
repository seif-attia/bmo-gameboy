#include "flappybird.h"
#include "controller.h"
#include "TFT.h"
#include "SPI.h"
#include "BIT_MATH.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>
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
#define INITIAL_GAP 68
#define INITIAL_SPACING 105
#define MIN_GAP 48
#define MAX_SPIKES 4

// Fixed-point scaling factor: 1 pixel = 16 sub-pixel units
#define FP_SHIFT 4
#define FP_SCALE (1 << FP_SHIFT)

// Dark Gray Cave Environment Palette (RGB565)
// "dark gray with some spots of even darker grey, nothing crazy"
#define COLOR_CAVE_GRAY 0x2104      // Clean dark slate gray (RGB 33, 33, 35)
#define COLOR_CAVE_DARK_SPOT 0x1082 // Even darker charcoal spot (RGB 17, 17, 18)

// Conical Stone Spikes Palette
#define COLOR_SPIKE_DARK 0x39E7  // Shaded right flank of stone cone
#define COLOR_SPIKE_BODY 0x5ACB  // Mid stone gray body
#define COLOR_SPIKE_LIGHT 0x8410 // Highlighted left flank of stone cone
#define COLOR_SPIKE_POINT 0xBDF7 // Sharp tip highlight point

// UI / Vampire Accents
#define COLOR_VAMP_RED 0xF800      // Bright blood red
#define COLOR_VAMP_DARK_RED 0x6800 // Deep blood crimson
#define COLOR_VAMP_PALE 0xDEFB     // Marceline pale skin accent

// Marceline 16-Color Palette
// True to Marceline from Adventure Time:
// 0: Transparent/Cave  1: Jet Black Raven Hair  2: Hair Sheen Dark Navy  3: Pale Bluish-Gray Skin
// 4: Red Vampire Bite Dots  5: Charcoal Gray Tank Top  6: Tank Top Highlight  7: Blue Denim Jeans
// 8: Deep Denim Blue Shadow  9: Brown Leather Boots  10: Crimson Bass Body  11: Bass White Pickguard
// 12: Silver Strings/Pegs  13: Black details (eye pupils, mouth, guitar neck)  14: White fangs / eye whites
static const uint16_t marcy_palette[16] PROGMEM = {
    0x0000, // 0: Transparent
    0x0000, // 1: Jet Black Raven Hair
    0x18C5, // 2: Hair Sheen Navy
    0xCE79, // 3: Marceline Pale Blue-Gray Skin (RGB 206, 207, 206)
    0xF800, // 4: Blood Red Vampire Bite Marks / Eyes
    0x3186, // 5: Dark Gray Tank Top
    0x528A, // 6: Tank Top Gray Highlight
    0x2A54, // 7: Classic Blue Denim Jeans
    0x118C, // 8: Deep Denim Shadow
    0x59A4, // 9: Brown Leather Boots
    0xD800, // 10: Crimson Red Axe Bass Body
    0xFFFF, // 11: Bass White Pickguard / Sparkle
    0xBDF7, // 12: Silver Strings / Neck Frets
    0x0841, // 13: Dark Outline / Details
    0xFFFF, // 14: White Fangs / Eye White
    0x8800  // 15: Deep Red Shadow on Axe Bass
};

// 14x14 High-Fidelity Marceline Sprite: Floating / Gliding Pose
// Showing long flowing black hair, pale face with red bite marks on her neck,
// dark gray tank top, blue jeans, brown boots, and her iconic Crimson Axe Bass slung at her side!
// 14x14 = 196 pixels = 98 bytes (4 bits per pixel, 2 pixels per byte)
static const uint8_t marceline_float_sprite[98] PROGMEM = {
    // Row 0: Top of hair puff
    0x00, 0x01, 0x11, 0x10, 0x00, 0x00, 0x00,
    // Row 1: Flowing hair & bangs
    0x00, 0x11, 0x21, 0x11, 0x10, 0x00, 0x00,
    // Row 2: Hair framing face, pale skin, black eye
    0x01, 0x13, 0x33, 0x14, 0x11, 0x00, 0x00,
    // Row 3: Marceline smiling face profile, fang, neck
    0x01, 0x13, 0x3E, 0x31, 0x11, 0x00, 0x00,
    // Row 4: Neck with signature 2 red bite marks, hair flowing back
    0x11, 0x11, 0x34, 0x41, 0x11, 0x00, 0x00,
    // Row 5: Tank top torso + silver guitar neck start
    0x12, 0x11, 0x56, 0x51, 0x1C, 0xC0, 0x00,
    // Row 6: Tank top + Axe Bass body upper horn
    0x11, 0x11, 0x55, 0x5A, 0xAC, 0x00, 0x00,
    // Row 7: Waist + Axe Bass crimson body & white pickguard
    0x01, 0x11, 0x77, 0xAB, 0xBA, 0x00, 0x00,
    // Row 8: Denim jeans + Axe Bass blade edge
    0x00, 0x11, 0x78, 0xAA, 0xFA, 0x00, 0x00,
    // Row 9: Flowing legs floating back + bottom of Axe Bass
    0x00, 0x01, 0x17, 0x7A, 0xAA, 0x00, 0x00,
    // Row 10: Lower legs & boots
    0x00, 0x00, 0x17, 0x89, 0x90, 0x00, 0x00,
    // Row 11: Brown boots trailing back
    0x00, 0x00, 0x01, 0x99, 0x90, 0x00, 0x00,
    // Row 12: Long raven hair tips dragging below
    0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00,
    // Row 13: Final hair curl
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00};

// 14x14 Marceline Rising / Flapping Pose (hair and legs trail back dynamically as she ascends)
static const uint8_t marceline_rise_sprite[98] PROGMEM = {
    // Row 0: Hair sweeps back higher
    0x01, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00,
    // Row 1: Flowing hair & bangs
    0x11, 0x21, 0x11, 0x11, 0x00, 0x00, 0x00,
    // Row 2: Face forward looking up, pale skin, red eyes
    0x11, 0x13, 0x33, 0x14, 0x10, 0x00, 0x00,
    // Row 3: Face profile + fang
    0x12, 0x13, 0x3E, 0x31, 0x10, 0x00, 0x00,
    // Row 4: Neck with 2 red bite marks
    0x11, 0x11, 0x34, 0x41, 0x10, 0x00, 0x00,
    // Row 5: Tank top + guitar neck tilted back
    0x11, 0x15, 0x65, 0x1C, 0xC0, 0x00, 0x00,
    // Row 6: Tank top + Axe Bass body
    0x01, 0x15, 0x55, 0xAA, 0xCC, 0x00, 0x00,
    // Row 7: Waist + Axe Bass body & white pickguard
    0x00, 0x17, 0x7A, 0xBB, 0xAA, 0x00, 0x00,
    // Row 8: Denim jeans swept back + blade
    0x00, 0x17, 0x8A, 0xAA, 0xFA, 0x00, 0x00,
    // Row 9: Swept legs + boots
    0x00, 0x11, 0x77, 0x8A, 0xAA, 0x00, 0x00,
    // Row 10: Floating boots
    0x00, 0x01, 0x17, 0x99, 0x90, 0x00, 0x00,
    // Row 11: Boots
    0x00, 0x00, 0x19, 0x90, 0x00, 0x00, 0x00,
    // Row 12: Long hair strands sweeping back
    0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Row 13:
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef struct
{
    int32_t x_fp;   // Sub-pixel X coordinate (x * 16)
    uint8_t gap_y;  // Top coordinate of the gap
    uint8_t gap_h;  // Height of the gap
    uint8_t active; // Whether the spike pillar is currently on screen
    uint8_t passed;
} SpikePillar;

static SpikePillar spikes[MAX_SPIKES];
static uint16_t score = 0;
static uint16_t prev_score = 0;
static uint16_t high_score = 0;

static uint16_t spike_speed = 16;
static uint8_t current_gap = INITIAL_GAP;
static uint8_t spike_spacing = INITIAL_SPACING;

// "dark gray with some spots of even darker grey, nothing crazy"
static inline uint16_t get_cave_pixel(uint8_t x, uint8_t y)
{
    // Subtle, clean dark gray texture: occasional dark charcoal mineral spots
    uint8_t spot = (uint8_t)((x * 37) ^ (y * 29));
    if ((spot & 0x1F) == 7)
    {
        return COLOR_CAVE_DARK_SPOT;
    }
    return COLOR_CAVE_GRAY;
}

static void spawn_spike_pillar(uint8_t index, int16_t start_x)
{
    spikes[index].x_fp = (int32_t)start_x * FP_SCALE;
    spikes[index].gap_h = current_gap;

    uint8_t min_y = 12;
    uint8_t max_y = TFT_HEIGHT - 12 - spikes[index].gap_h;
    spikes[index].gap_y = min_y + (rand() % (max_y - min_y + 1));

    spikes[index].active = 1;
    spikes[index].passed = 0;
}

static void init_spikes(void)
{
    score = 0;
    prev_score = 0;
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
    if (spike_speed > 36)
        spike_speed = 36;

    int16_t temp_gap = INITIAL_GAP - (score / 3);
    if (temp_gap < MIN_GAP)
        current_gap = MIN_GAP;
    else
        current_gap = (uint8_t)temp_gap;

    int16_t temp_spacing = INITIAL_SPACING - (score / 2);
    if (temp_spacing < 65)
        spike_spacing = 65;
    else
        spike_spacing = (uint8_t)temp_spacing;
}

// FULLY CONICAL STONE SPIKES:
// A true cone where width expands linearly from 0 at the tip (gap boundary) to full base width at screen edge!
// col_idx: 0 .. (SPIKE_WIDTH - 1)
// is_upper: true for stalactite hanging from ceiling pointing down; false for stalagmite rising from floor pointing up
// y: current screen row
// tip_y: row where the sharp point meets the gap
static uint16_t get_conical_spike_pixel(uint8_t col_idx, uint8_t y, uint8_t tip_y, bool is_upper)
{
    // Distance from the tip of the cone towards its base
    uint8_t dist_from_tip = is_upper ? (tip_y - y) : (y - tip_y);
    uint8_t cone_height = is_upper ? tip_y : (TFT_HEIGHT - 1 - tip_y);

    if (cone_height == 0)
        cone_height = 1;

    // Linear cone expansion: radius(d) = (d * (SPIKE_WIDTH / 2)) / cone_height
    // At tip (dist=0): radius = 0 (only sharp central point)
    // At base (dist=cone_height): radius = SPIKE_WIDTH / 2 (full 20px base at the ceiling/floor)
    int8_t half_width = (int8_t)(((uint16_t)dist_from_tip * (SPIKE_WIDTH / 2)) / cone_height);
    int8_t center_dist = abs((int8_t)col_idx - (SPIKE_WIDTH / 2));

    // Outside the conical silhouette: render dark gray cave air
    if (center_dist > half_width)
    {
        return get_cave_pixel(0, y);
    }

    // Razor-sharp needle point right at the tip
    if (dist_from_tip <= 2)
    {
        return COLOR_SPIKE_POINT;
    }

    // Outer edge boundary facet
    if (center_dist == half_width)
    {
        return (col_idx < (SPIKE_WIDTH / 2)) ? COLOR_SPIKE_LIGHT : COLOR_SPIKE_DARK;
    }

    // Chiseled conical stone shading:
    // Left side illuminated by ambient light, right side shaded
    int8_t normalized = (int8_t)col_idx - (SPIKE_WIDTH / 2); // -half_width .. +half_width
    if (normalized < -(half_width / 2))
        return COLOR_SPIKE_LIGHT;
    else if (normalized > (half_width / 2))
        return COLOR_SPIKE_DARK;
    else
        return COLOR_SPIKE_BODY;
}

// Draw a single vertical column of the dark gray cave background
static void draw_cave_column(int16_t x)
{
    if (x < 0 || x >= TFT_WIDTH)
        return;

    TFT_SetAddrWindow((uint8_t)x, 0, (uint8_t)x, TFT_HEIGHT - 1);
    TFT_CS_LOW();
    TFT_DC_HIGH();

    for (uint8_t y = 0; y < TFT_HEIGHT; y++)
    {
        TFT_spiWrite16(get_cave_pixel((uint8_t)x, y));
    }
    TFT_CS_HIGH();
}

// Draw a single vertical column of a fully conical stone spike pillar
static void draw_spike_column(int16_t x, uint8_t col_idx, uint8_t gap_y, uint8_t gap_h)
{
    if (x < 0 || x >= TFT_WIDTH)
        return;

    TFT_SetAddrWindow((uint8_t)x, 0, (uint8_t)x, TFT_HEIGHT - 1);
    TFT_CS_LOW();
    TFT_DC_HIGH();

    // 1. Upper hanging conical stalactite (from ceiling y=0 down to gap_y)
    for (uint8_t y = 0; y < gap_y; y++)
    {
        TFT_spiWrite16(get_conical_spike_pixel(col_idx, y, gap_y - 1, true));
    }

    // 2. Open air gap for Marceline
    for (uint8_t y = gap_y; y < gap_y + gap_h; y++)
    {
        TFT_spiWrite16(get_cave_pixel((uint8_t)x, y));
    }

    // 3. Lower rising conical stalagmite (from gap_y + gap_h down to floor)
    uint8_t lower_tip = gap_y + gap_h;
    for (uint8_t y = lower_tip; y < TFT_HEIGHT; y++)
    {
        TFT_spiWrite16(get_conical_spike_pixel(col_idx, y, lower_tip, false));
    }

    TFT_CS_HIGH();
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

        // Score update when Marceline safely passes the spike
        if (!spikes[i].passed && (new_x + SPIKE_WIDTH < marcy_x))
        {
            spikes[i].passed = 1;
            prev_score = score;
            score++;
            play_sfx(sfx_flappy_score);
            if (score > high_score)
            {
                high_score = score;
            }
        }

        // Incremental rendering for smooth 60 FPS motion
        for (int16_t x = new_x; x < old_x; x++)
        {
            // Draw newly revealed column of the conical spike on the left
            for (uint8_t c = 0; c < SPIKE_WIDTH; c++)
            {
                int16_t col_x = x + c;
                if (col_x >= 0 && col_x < TFT_WIDTH)
                {
                    draw_spike_column(col_x, c, spikes[i].gap_y, spikes[i].gap_h);
                }
            }

            // Restore dark gray cave background on trailing edge on the right
            int16_t clear_x = x + SPIKE_WIDTH;
            if (clear_x >= 0 && clear_x < TFT_WIDTH)
            {
                draw_cave_column(clear_x);
            }
        }

        // Multi-pillar spawn check
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

        // Recycle pillar off left edge
        if (new_x + SPIKE_WIDTH < 0)
        {
            spikes[i].active = 0;
        }
    }
}

// Draw Marceline character model (14x14)
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

            // Pixel 1
            if (p1 == 0)
                TFT_spiWrite16(get_cave_pixel((uint8_t)(px + x), (uint8_t)(py + y)));
            else
                TFT_spiWrite16(pgm_read_word(&marcy_palette[p1]));

            // Pixel 2
            if (p2 == 0)
                TFT_spiWrite16(get_cave_pixel((uint8_t)(px + x + 1), (uint8_t)(py + y)));
            else
                TFT_spiWrite16(pgm_read_word(&marcy_palette[p2]));
        }
    }
    TFT_CS_HIGH();
}

// Erase previous Marceline position by restoring exact cave backdrop
static void erase_marceline(int16_t px, int16_t py)
{
    if (px >= TFT_WIDTH || py >= TFT_HEIGHT || (px + MARCY_W) <= 0 || (py + MARCY_H) <= 0)
        return;

    TFT_SetAddrWindow((uint8_t)px, (uint8_t)py, (uint8_t)(px + MARCY_W - 1), (uint8_t)(py + MARCY_H - 1));
    TFT_CS_LOW();
    TFT_DC_HIGH();

    for (uint8_t y = 0; y < MARCY_H; y++)
    {
        for (uint8_t x = 0; x < MARCY_W; x++)
        {
            TFT_spiWrite16(get_cave_pixel((uint8_t)(px + x), (uint8_t)(py + y)));
        }
    }
    TFT_CS_HIGH();
}

static void draw_score(void)
{
    char score_str[8];
    if (prev_score != score)
    {
        sprintf(score_str, "%u", prev_score);
        TFT_DrawString(TFT_WIDTH / 2 - 8, 6, score_str, COLOR_CAVE_GRAY, COLOR_CAVE_GRAY, 2);
    }

    sprintf(score_str, "%u", score);
    TFT_DrawString(TFT_WIDTH / 2 - 8, 6, score_str, COLOR_VAMP_RED, COLOR_CAVE_GRAY, 2);

    prev_score = score;
}

// Conical AABB & Profile Collision Check:
// Accurate collision checking considering the conical taper of stone spikes!
static uint8_t check_spike_collision(uint8_t marcy_x, uint8_t marcy_y)
{
    for (uint8_t i = 0; i < MAX_SPIKES; i++)
    {
        if (!spikes[i].active)
            continue;

        int16_t spike_x = (int16_t)(spikes[i].x_fp >> FP_SHIFT);

        // Check horizontal overlap between Marceline (14x14) and Spike (width 20)
        // With tight 2px inset on Marceline's bounding box
        if ((marcy_x + MARCY_W - 3) >= spike_x && (marcy_x + 3) <= (spike_x + SPIKE_WIDTH))
        {
            // Upper stalactite collision check
            uint8_t upper_tip = spikes[i].gap_y;
            if (marcy_y < upper_tip)
            {
                // Find horizontal penetration into the cone
                uint8_t dist_from_tip = upper_tip - marcy_y;
                int8_t half_w = (int8_t)(((uint16_t)dist_from_tip * (SPIKE_WIDTH / 2)) / (upper_tip == 0 ? 1 : upper_tip));
                int8_t center_dist = abs((int8_t)(marcy_x + (MARCY_W / 2)) - (spike_x + (SPIKE_WIDTH / 2)));
                if (center_dist <= half_w + 2)
                {
                    return 1;
                }
            }

            // Lower stalagmite collision check
            uint8_t lower_tip = spikes[i].gap_y + spikes[i].gap_h;
            if ((marcy_y + MARCY_H - 2) > lower_tip)
            {
                uint8_t dist_from_tip = (marcy_y + MARCY_H - 2) - lower_tip;
                uint8_t cone_h = TFT_HEIGHT - lower_tip;
                int8_t half_w = (int8_t)(((uint16_t)dist_from_tip * (SPIKE_WIDTH / 2)) / (cone_h == 0 ? 1 : cone_h));
                int8_t center_dist = abs((int8_t)(marcy_x + (MARCY_W / 2)) - (spike_x + (SPIKE_WIDTH / 2)));
                if (center_dist <= half_w + 2)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Centered text drawing helper
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

        // 1. Clean Dark Gray Cave Backdrop
        TFT_SetAddrWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
        TFT_CS_LOW();
        TFT_DC_HIGH();
        for (uint8_t y = 0; y < TFT_HEIGHT; y++)
        {
            for (uint8_t x = 0; x < TFT_WIDTH; x++)
            {
                TFT_spiWrite16(get_cave_pixel(x, y));
            }
        }
        TFT_CS_HIGH();

        uint8_t marcy_x = 26;
        int16_t marcy_y = TFT_HEIGHT / 2 - (MARCY_H / 2);
        int16_t marcy_y_prev = marcy_y;

        uint8_t end_game = false;
        int16_t velocity = 0;
        uint8_t gravity_counter = 0;
        uint32_t frame_timer = millis();

        // Title Splash Screen
        draw_string_centered(18, "MARCELINE'S ESCAPE", COLOR_VAMP_RED, COLOR_CAVE_GRAY, 1);
        draw_marceline(marcy_x, marcy_y, false);
        draw_string_centered(TFT_HEIGHT - 22, "PRESS START", COLOR_SPIKE_POINT, COLOR_CAVE_GRAY, 1);

        while (!get_key(START))
        {
            update_music();
            if (get_key(BACK))
            {
                play_again = false;
                break;
            }
        }

        if (!play_again)
            break;

        // Clear splash text
        TFT_FillRect(0, 16, TFT_WIDTH, 14, COLOR_CAVE_GRAY);
        TFT_FillRect(0, TFT_HEIGHT - 26, TFT_WIDTH, 14, COLOR_CAVE_GRAY);
        init_spikes();

        // 2. MAIN GAMEPLAY LOOP
        while (!end_game)
        {
            update_music();
            uint32_t current_time = millis();

            if (get_key(BACK))
            {
                end_game = true;
                play_again = false;
                break;
            }

            if (current_time - frame_timer >= 16) // 60 FPS tick
            {
                frame_timer = current_time;

                bool is_rising = false;
                if (get_key(UP))
                {
                    velocity = -3;
                    is_rising = true;
                }

                // Gravity Simulation
                gravity_counter++;
                if (gravity_counter >= 4)
                {
                    gravity_counter = 0;
                    if (velocity < 2)
                    {
                        velocity++;
                    }
                }

                // Erase previous Marceline position
                erase_marceline(marcy_x, marcy_y_prev);

                marcy_y += velocity;

                // Ceiling and floor bounds
                if (marcy_y < 1)
                {
                    marcy_y = 1;
                    velocity = 0;
                }
                if (marcy_y >= TFT_HEIGHT - MARCY_H - 1)
                {
                    marcy_y = TFT_HEIGHT - MARCY_H - 1;
                    play_sfx(sfx_gameover_1);
                    end_game = true;
                }

                // Update stone conical spikes
                update_and_draw_spikes(marcy_x);

                // Render Marceline at new position
                draw_marceline(marcy_x, marcy_y, is_rising || (velocity < 0));
                marcy_y_prev = marcy_y;

                draw_score();

                // Conical spike collision check
                if (check_spike_collision(marcy_x, (uint8_t)marcy_y))
                {
                    play_sfx(sfx_gameover_1);
                    end_game = true;
                }
            }
        }

        // 3. GAME OVER SCREEN (Clean Dark Cave Tomb Theme)
        if (play_again)
        {
            play_sfx(sfx_gameover_fast);
            TFT_FillScreen(COLOR_CAVE_DARK_SPOT);

            // Dialog Frame
            TFT_FillRect(12, 15, 136, 98, COLOR_CAVE_GRAY);
            TFT_DrawHLine_Fast(12, 15, 136, COLOR_SPIKE_POINT);
            TFT_DrawHLine_Fast(12, 112, 136, COLOR_SPIKE_POINT);
            TFT_DrawVLine_Fast(12, 15, 98, COLOR_SPIKE_POINT);
            TFT_DrawVLine_Fast(147, 15, 98, COLOR_SPIKE_POINT);

            TFT_DrawHLine_Fast(14, 17, 132, COLOR_VAMP_RED);
            TFT_DrawHLine_Fast(14, 110, 132, COLOR_VAMP_RED);
            TFT_DrawVLine_Fast(14, 17, 94, COLOR_VAMP_RED);
            TFT_DrawVLine_Fast(145, 17, 94, COLOR_VAMP_RED);

            // Header Banner
            TFT_FillRect(15, 18, 130, 22, COLOR_CAVE_DARK_SPOT);
            draw_string_centered(25, "TRAPPED IN THE CAVE!", COLOR_VAMP_RED, COLOR_CAVE_DARK_SPOT, 1);

            // Score Badges
            char score_buf[20];
            sprintf(score_buf, "SCORE: %u", score);
            draw_string_centered(48, score_buf, COLOR_SPIKE_POINT, COLOR_CAVE_GRAY, 1);

            sprintf(score_buf, "BEST : %u", high_score);
            draw_string_centered(60, score_buf, COLOR_VAMP_PALE, COLOR_CAVE_GRAY, 1);

            // Buttons
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
