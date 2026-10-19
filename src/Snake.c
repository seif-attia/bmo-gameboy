#include "Snake.h"
#include "TFT.h"
#include "SPI.h"
#include "BIT_MATH.h"
#include "controller.h"
#include "timer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "music.h"
#include "sfx.h"

#define TFT_CS_LOW() CLR_BIT(PORTB, TFT_CS)
#define TFT_CS_HIGH() SET_BIT(PORTB, TFT_CS)
#define TFT_DC_HIGH() SET_BIT(PORTB, TFT_DC)

#define GRID_SIZE 6
#define BOARD_WIDTH 26  // 26 * 6 = 156 px (centered on 160 px screen, 2px border each side)
#define BOARD_HEIGHT 19 // 19 * 6 = 114 px (starts at y=14, ends at y=128)
#define BOARD_OFFSET_X 2
#define BOARD_OFFSET_Y 14
#define MAX_SNAKE_LENGTH 50

// Palette definitions (RGB565)
// Candy Kingdom Environment Colors
#define CK_SKY_BLUE 0xB6DF       // Pastel fairy-tale sky blue (RGB 180, 218, 255)
#define CK_SKY_LIGHT 0xD77F      // Soft horizon sky
#define CK_PASTEL_CREAM 0xFFFA   // Marshmallow fluff / sugar cloud white
#define CK_CREAM_SHADOW 0xDE77   // Cloud underside shadow
#define CK_CASTLE_PINK 0xFCEF    // Sugar-frosted castle wall pink
#define CK_CASTLE_SHADE 0xEC8D   // Castle tower shadow pink
#define CK_CONE_BROWN 0xCB06     // Waffle cone brown
#define CK_CONE_DARK 0x9261      // Waffle cone waffle grid dark brown
#define CK_PEPPERMINT_RED 0xF800 // Candy cane striping red
#define CK_BANNER_MAGENTA 0x8010 // Princess Bubblegum royal magenta
#define CK_BANNER_BORDER 0xFE16  // Bubblegum neon border

// Ground / Hill Tones (Layered sugar hills)
#define CK_HILL_TOP 0xFD75      // Bubblegum rolling hill crest
#define CK_GROUND_BASE 0xFD34   // Sweet meadow pink
#define CK_RIVER_CREAM 0x879F   // Soda / syrup river pastel blue
#define CK_RIVER_DEEP 0x559D    // Soda river depth
#define CK_ROAD_COOKIE 0xEDCB   // Sugar wafer path / cookie dust
#define CK_ROAD_DARK 0xD485     // Cookie crumb shadow
#define CK_ICING_WHITE 0xFFFF   // Whipped icing / snow crests
#define CK_SPRINKLE_CYAN 0x3EFD // Candy sprinkle cyan
#define CK_SPRINKLE_LIME 0x87E4 // Lime green sugar crystal
#define CK_SPRINKLE_YEL 0xFFE0  // Yellow sugar crystal
#define CK_SPRINKLE_PURP 0xA177 // Violet sparkle

// Jake the Dog 16-Color Palette
// 0: Transparent/Sky  1: Bright Jake Yellow  2: Jake Deep Gold  3: Jake Highlight Yellow
// 4: White (Eyes/Teeth)  5: Black (Pupils/Nose)  6: Muzzle Contour  7: Ear Shadow
static const uint16_t jake_palette[8] PROGMEM = {
    0x0000, // 0: (unused / transparent)
    0xFDE0, // 1: Jake Bright Gold (RGB 253, 189, 0)
    0xD400, // 2: Jake Dark Gold / Contour
    0xFFE6, // 3: Jake Light Highlight
    0xFFFF, // 4: White (Eyes)
    0x0000, // 5: Black (Eyes, Nose, Jowls)
    0xFA40, // 6: Warm Amber Jowl
    0xBD00  // 7: Deep Shadow Gold
};

// Jake Head Directional 6x6 Sprites (4 bits per pixel, 2 pixels per byte = 18 bytes each)
// DIR_RIGHT: Big iconic wide white eye with black pupil forward, drooping jowl, black snout tip
static const uint8_t jake_head_right[18] PROGMEM = {
    0x21, 0x11, 0x12, // . J J J J .
    0x24, 0x51, 0x17, // (W B) J J ear
    0x14, 0x51, 0x15, // (W B) J J Nose
    0x21, 0x66, 0x62, // . J (jowls) .
    0x24, 0x51, 0x15, // (W B) J J Muzzle
    0x72, 0x11, 0x27  // ear J J J .
};

// DIR_LEFT: Big white eye looking left, snout facing left
static const uint8_t jake_head_left[18] PROGMEM = {
    0x21, 0x11, 0x12, // . J J J J .
    0x71, 0x15, 0x42, // ear J J (B W)
    0x51, 0x15, 0x41, // Nose J J (B W)
    0x26, 0x66, 0x12, // . (jowls) J .
    0x51, 0x15, 0x42, // Muzzle J J (B W)
    0x72, 0x11, 0x27  // . J J J ear
};

// DIR_UP: Both huge expressive eyes looking upward, snout between them
static const uint8_t jake_head_up[18] PROGMEM = {
    0x21, 0x55, 0x12, // . J (snout) J .
    0x45, 0x11, 0x54, // (W B) J J (B W)
    0x45, 0x11, 0x54, // (W B) J J (B W)
    0x21, 0x66, 0x12, // . J (jowls) J .
    0x71, 0x11, 0x17, // Ear J J J J Ear
    0x02, 0x11, 0x20  // . . J J . .
};

// DIR_DOWN: Jake looking downward towards treats, nose at bottom
static const uint8_t jake_head_down[18] PROGMEM = {
    0x02, 0x11, 0x20, // . . J J . .
    0x71, 0x11, 0x17, // Ear J J J J Ear
    0x45, 0x11, 0x54, // (W B) J J (B W)
    0x45, 0x11, 0x54, // (W B) J J (B W)
    0x21, 0x66, 0x12, // . J (jowls) J .
    0x21, 0x55, 0x12  // . J (snout) J .
};

// Jake Body & Tail (6x6 pixels, 18 bytes each)
static const uint8_t jake_body_sprite[18] PROGMEM = {
    0x21, 0x33, 0x12, // . J (hi hi) J .
    0x13, 0x11, 0x31, // J (hi) J J (hi) J
    0x11, 0x11, 0x11, // J J J J J J
    0x11, 0x11, 0x11, // J J J J J J
    0x21, 0x22, 0x12, // . J (sh sh) J .
    0x72, 0x22, 0x27  // ear . (sh sh) . ear
};

// Jake Tail: Cute rounded little bulldog tail
static const uint8_t jake_tail_sprite[18] PROGMEM = {
    0x00, 0x22, 0x00, // . . (gold) . .
    0x02, 0x11, 0x20, // . (gold J J gold) .
    0x21, 0x31, 0x12, // gold J (hi) J J gold
    0x21, 0x11, 0x12, // gold J J J J gold
    0x02, 0x11, 0x20, // . (gold J J gold) .
    0x00, 0x22, 0x00  // . . (gold) . .
};

// 5 Highly Detailed 6x6 Candy Sprites (16-color custom palette)
// 0: Transparent/Background  1: Red  2: White  3: Deep Pink  4: Light Pink
// 5: Gloss Sparkle White  6: Lemon Yellow  7: Deep Gold/Caramel
// 8: Royal Purple  9: Lilac  10: Sky Blue  11: Cyan Ice  12: Mint Green
// 13: Chocolate Brown  14: Dark Shadow  15: Cookie Tan
static const uint16_t candy_palette[16] PROGMEM = {
    0x0000, // 0: Keyed transparent
    0xF800, // 1: Cherry / Peppermint Red
    0xFFFF, // 2: Pure White
    0xF935, // 3: Bubblegum Rich Pink
    0xFE39, // 4: Soft Cotton Candy Pink
    0xFFFF, // 5: Gloss Highlight Sparkle
    0xFFE0, // 6: Lemon Citrus Yellow
    0xD400, // 7: Golden Honey / Caramel
    0x88F5, // 8: Royal Grape Jelly Purple
    0xBA7B, // 9: Sweet Lilac Lavender
    0x05BF, // 10: Soda Cyan / Blue Raspberry
    0x87FF, // 11: Frosting Glaze Cyan
    0x3FE8, // 12: Spearmint Green
    0x61A2, // 13: Fudge Chocolate
    0x30A5, // 14: Deep Shadow
    0xE571  // 15: Graham Cookie Base
};

// Candy 0: Peppermint Butler Swirl (Spiraling red and white candy medallion with shine)
static const uint8_t candy_peppermint[18] PROGMEM = {
    0x02, 0x11, 0x20, // . W R R W .
    0x21, 0x22, 0x12, // W R W W R W
    0x12, 0x51, 0x21, // R W (shine) R W R
    0x12, 0x15, 0x21, // R W R (shine) W R
    0x21, 0x22, 0x12, // W R W W R W
    0x02, 0x11, 0x20  // . W R R W .
};

// Candy 1: Princess Bubblegum Jewel Gumdrop (Sugar crystal dome with glistening facet)
static const uint8_t candy_gumdrop[18] PROGMEM = {
    0x00, 0x44, 0x00, // . . P P . .
    0x04, 0x53, 0x30, // . P (shine) P P .
    0x43, 0x53, 0x33, // P P (shine) P P P
    0x33, 0x33, 0x3E, // P P P P P (shadow)
    0x33, 0x3E, 0xEE, // P P P (shadow shadow shadow)
    0x0E, 0xEE, 0xE0  // . (shadow shadow shadow shadow) .
};

// Candy 2: Lemon Cream Hard Candy (Glowing lemon wrapped drop with caramel rim)
static const uint8_t candy_lemon[18] PROGMEM = {
    0x00, 0x76, 0x00, // . . (gold Y) . .
    0x07, 0x56, 0x60, // . gold (shine) Y Y .
    0x76, 0x56, 0x67, // gold Y (shine) Y Y gold
    0x76, 0x66, 0x77, // gold Y Y Y gold gold
    0x07, 0x67, 0x70, // . gold Y gold gold .
    0x00, 0x77, 0x00  // . . gold gold . .
};

// Candy 3: Royal Grape Jelly Bean (Curved glossy jelly bean)
static const uint8_t candy_jellybean[18] PROGMEM = {
    0x08, 0x89, 0x00, // . Pur Pur Lilac . .
    0x89, 0x59, 0x80, // Pur Lilac (shine) Lilac Pur .
    0x88, 0x98, 0x88, // Pur Pur Lilac Pur Pur Pur
    0x88, 0x88, 0x8E, // Pur Pur Pur Pur Pur Shd
    0x08, 0x88, 0xEE, // . Pur Pur Pur Shd Shd
    0x00, 0x8E, 0xE0  // . . Pur Shd Shd .
};

// Candy 4: Blue Raspberry Cotton Candy Star / Bonbon
static const uint8_t candy_raspberry[18] PROGMEM = {
    0x00, 0xB5, 0x00, // . . Cy (shine) . .
    0x0A, 0xBB, 0xB0, // . Bl Cy Cy Cy .
    0xB5, 0xBA, 0xAA, // Cy (shine) Cy Bl Bl Bl
    0xAA, 0xAB, 0xAE, // Bl Bl Bl Cy Bl Shd
    0x0A, 0xAA, 0xEE, // . Bl Bl Bl Shd Shd
    0x00, 0xAE, 0xE0  // . . Bl Shd Shd .
};

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
static uint8_t food_type = 0;
static uint16_t score = 0;
static uint32_t move_interval = 250;

// Function to calculate exact Candy Kingdom scenery color at any pixel (x, y)
// This gives an authentic Adventure Time landscape that Jake moves across:
// - Blue sugar sky with puffy marshmallow clouds
// - Princess Bubblegum's Palace (candy towers, waffle cone roofs, peppermint stripes)
// - Rolling hills of pink frosting with layered candy cane pathways, soda rivers, & sprinkles
static uint16_t get_candy_kingdom_pixel(uint8_t x, uint8_t y)
{
    // Region 1: Sky & Sugar Clouds (y: 14 .. 33)
    if (y < 34)
    {
        // Marshmallow Sugar Clouds (Clusters across the horizon)
        // Cloud 1: left cluster
        if ((x >= 8 && x <= 36 && y >= 17 && y <= 25) ||
            (x >= 14 && x <= 30 && y >= 15 && y <= 27))
        {
            if (y >= 26)
                return CK_CREAM_SHADOW;
            return CK_PASTEL_CREAM;
        }
        // Cloud 2: right cluster
        if ((x >= 118 && x <= 152 && y >= 16 && y <= 24) ||
            (x >= 126 && x <= 144 && y >= 14 && y <= 26))
        {
            if (y >= 25)
                return CK_CREAM_SHADOW;
            return CK_PASTEL_CREAM;
        }

        // Sky gradient
        return (y > 28) ? CK_SKY_LIGHT : CK_SKY_BLUE;
    }

    // Region 2: Candy Kingdom Castle Skyline (y: 34 .. 62)
    // Central Main Palace Keep (x: 64 .. 96)
    if (x >= 64 && x <= 96 && y >= 34 && y <= 62)
    {
        // Giant central waffle cone spire (x: 74 .. 86, y: 34 .. 46)
        if (y <= 46)
        {
            int8_t cone_half_w = (int8_t)(y - 34); // starts at 0 at y=34, expands to 12 at y=46
            int8_t center_dist = abs((int8_t)x - 80);
            if (center_dist <= cone_half_w)
            {
                // Waffle cone lattice pattern
                if (((x + y) % 4 == 0) || ((x - y + 64) % 4 == 0))
                    return CK_CONE_DARK;
                return CK_CONE_BROWN;
            }
        }
        // Castle walls with candy windows & battlements
        if (y >= 47)
        {
            // Peppermint candy cane towers on flanks of the keep
            if (x <= 68 || x >= 92)
            {
                if ((x + y) % 6 < 3)
                    return CK_PEPPERMINT_RED;
                return CK_ICING_WHITE;
            }
            // Royal palace window
            if (x >= 77 && x <= 83 && y >= 50 && y <= 57)
            {
                if (y == 50 || x == 77 || x == 83)
                    return CK_CONE_DARK;
                return CK_BANNER_MAGENTA;
            }
            // Shading on wall
            if (x >= 80)
                return CK_CASTLE_SHADE;
            return CK_CASTLE_PINK;
        }
    }

    // Left Flank Tower (x: 42 .. 54, y: 40 .. 60)
    if (x >= 42 && x <= 54 && y >= 40 && y <= 60)
    {
        if (y <= 48)
        {
            // Small ice-cream cone turret
            int8_t dist = abs((int8_t)x - 48);
            if (dist <= (y - 40))
                return CK_CONE_BROWN;
        }
        else
        {
            if ((x + y) % 5 < 2)
                return CK_PEPPERMINT_RED;
            return CK_ICING_WHITE;
        }
    }

    // Right Flank Tower (x: 106 .. 118, y: 40 .. 60)
    if (x >= 106 && x <= 118 && y >= 40 && y <= 60)
    {
        if (y <= 48)
        {
            int8_t dist = abs((int8_t)x - 112);
            if (dist <= (y - 40))
                return CK_CONE_BROWN;
        }
        else
        {
            if ((x + y) % 5 < 2)
                return CK_PEPPERMINT_RED;
            return CK_ICING_WHITE;
        }
    }

    // Background hills behind towers
    if (y < 63)
    {
        // Far purple/pink sugar hills
        return ((x + y) % 2 == 0) ? CK_CASTLE_SHADE : CK_HILL_TOP;
    }

    // Region 3: Candy Kingdom Rolling Frosting Hills & Sugar Soda River (y: 63 .. 127)
    // Sweet soda river winding through the candy landscape
    // River path: curves from x=135 at y=63 down through x=80 at y=95 to x=25 at y=127
    int16_t river_center = 135 - (((int16_t)(y - 63) * 110) >> 6);
    if (abs((int16_t)x - river_center) <= 5)
    {
        // Soda stream with sparkling foam
        if (abs((int16_t)x - river_center) == 5)
            return CK_ICING_WHITE;
        if ((x + y) % 5 == 0)
            return CK_ICING_WHITE;
        if ((x + y) % 2 == 0)
            return CK_RIVER_CREAM;
        return CK_RIVER_DEEP;
    }

    // Graham cracker / sugar wafer walkway
    int16_t road_center = 25 + (((int16_t)(y - 63) * 60) >> 6);
    if (abs((int16_t)x - road_center) <= 4)
    {
        if ((x % 3 == 0) && (y % 3 == 0))
            return CK_ROAD_DARK;
        return CK_ROAD_COOKIE;
    }

    // Rolling pink frosting hills with icing crests & candy sprinkles
    // Hill contour line
    if (y == 78 || y == 102)
        return CK_ICING_WHITE;

    // Scattered candy sprinkles across the meadows
    uint8_t hash = (uint8_t)(x * 37 + y * 19);
    if (hash == 42)
        return CK_SPRINKLE_CYAN;
    if (hash == 85)
        return CK_SPRINKLE_LIME;
    if (hash == 128)
        return CK_SPRINKLE_YEL;
    if (hash == 171)
        return CK_SPRINKLE_PURP;

    // Luscious layered pink cake terrain
    if (y < 85)
        return ((x + y) % 2 == 0) ? CK_HILL_TOP : CK_GROUND_BASE;
    else if (y < 110)
        return ((x / 3 + y / 3) % 2 == 0) ? CK_GROUND_BASE : CK_HILL_TOP;
    else
        return ((x + y) % 3 == 0) ? CK_GROUND_BASE : CK_HILL_TOP;
}

// Redraws a single 6x6 board grid cell restoring its exact underlying Candy Kingdom scenery
static void restore_candy_kingdom_cell(int8_t gx, int8_t gy)
{
    uint8_t px = BOARD_OFFSET_X + (gx * GRID_SIZE);
    uint8_t py = BOARD_OFFSET_Y + (gy * GRID_SIZE);

    TFT_SetAddrWindow(px, py, px + GRID_SIZE - 1, py + GRID_SIZE - 1);
    TFT_CS_LOW();
    TFT_DC_HIGH();

    for (uint8_t y = py; y < py + GRID_SIZE; y++)
    {
        for (uint8_t x = px; x < px + GRID_SIZE; x++)
        {
            TFT_spiWrite16(get_candy_kingdom_pixel(x, y));
        }
    }
    TFT_CS_HIGH();
}

// Full redraw of the Candy Kingdom play area and Top HUD Banner
static void draw_candy_kingdom_full_scene(void)
{
    // 1. Top HUD Banner (0 .. 13)
    TFT_FillRect(0, 0, TFT_WIDTH, 14, CK_BANNER_MAGENTA);
    TFT_DrawHLine_Fast(0, 13, TFT_WIDTH, CK_BANNER_BORDER);

    // Header labels
    TFT_DrawString(4, 3, "JAKE", 0xFDE0, CK_BANNER_MAGENTA, 1);
    TFT_DrawString(32, 3, "pts:", 0xFFFF, CK_BANNER_MAGENTA, 1);
    TFT_DrawString(60, 3, "0", 0xFFE0, CK_BANNER_MAGENTA, 1);
    TFT_DrawString(110, 3, "CANDY", 0xFE39, CK_BANNER_MAGENTA, 1);

    // Left and Right screen margins (2px each)
    TFT_FillRect(0, BOARD_OFFSET_Y, BOARD_OFFSET_X, TFT_HEIGHT - BOARD_OFFSET_Y, CK_BANNER_MAGENTA);
    TFT_FillRect(TFT_WIDTH - BOARD_OFFSET_X, BOARD_OFFSET_Y, BOARD_OFFSET_X, TFT_HEIGHT - BOARD_OFFSET_Y, CK_BANNER_MAGENTA);

    // 2. Stream all playfield pixels directly using hardware SPI
    TFT_SetAddrWindow(BOARD_OFFSET_X, BOARD_OFFSET_Y,
                      BOARD_OFFSET_X + (BOARD_WIDTH * GRID_SIZE) - 1,
                      BOARD_OFFSET_Y + (BOARD_HEIGHT * GRID_SIZE) - 1);

    TFT_CS_LOW();
    TFT_DC_HIGH();

    for (uint8_t y = BOARD_OFFSET_Y; y < BOARD_OFFSET_Y + (BOARD_HEIGHT * GRID_SIZE); y++)
    {
        for (uint8_t x = BOARD_OFFSET_X; x < BOARD_OFFSET_X + (BOARD_WIDTH * GRID_SIZE); x++)
        {
            TFT_spiWrite16(get_candy_kingdom_pixel(x, y));
        }
    }
    TFT_CS_HIGH();
}

// Draw Jake Head (6x6 sprite with transparent background matching terrain)
static void draw_jake_head(int8_t gx, int8_t gy, Direction dir)
{
    const uint8_t *sprite;
    switch (dir)
    {
    case DIR_UP:
        sprite = jake_head_up;
        break;
    case DIR_DOWN:
        sprite = jake_head_down;
        break;
    case DIR_LEFT:
        sprite = jake_head_left;
        break;
    case DIR_RIGHT:
    default:
        sprite = jake_head_right;
        break;
    }

    uint8_t px = BOARD_OFFSET_X + (gx * GRID_SIZE);
    uint8_t py = BOARD_OFFSET_Y + (gy * GRID_SIZE);

    TFT_SetAddrWindow(px, py, px + 5, py + 5);
    TFT_CS_LOW();
    TFT_DC_HIGH();

    uint8_t byte_idx = 0;
    for (uint8_t y = 0; y < 6; y++)
    {
        for (uint8_t x = 0; x < 6; x += 2)
        {
            uint8_t b = pgm_read_byte(&sprite[byte_idx++]);
            uint8_t p1 = b >> 4;
            uint8_t p2 = b & 0x0F;

            // Pixel 1
            if (p1 == 0)
                TFT_spiWrite16(get_candy_kingdom_pixel(px + x, py + y));
            else
                TFT_spiWrite16(pgm_read_word(&jake_palette[p1]));

            // Pixel 2
            if (p2 == 0)
                TFT_spiWrite16(get_candy_kingdom_pixel(px + x + 1, py + y));
            else
                TFT_spiWrite16(pgm_read_word(&jake_palette[p2]));
        }
    }
    TFT_CS_HIGH();
}

// Draw Jake Body or Tail
static void draw_jake_segment(int8_t gx, int8_t gy, bool is_tail)
{
    const uint8_t *sprite = is_tail ? jake_tail_sprite : jake_body_sprite;
    uint8_t px = BOARD_OFFSET_X + (gx * GRID_SIZE);
    uint8_t py = BOARD_OFFSET_Y + (gy * GRID_SIZE);

    TFT_SetAddrWindow(px, py, px + 5, py + 5);
    TFT_CS_LOW();
    TFT_DC_HIGH();

    uint8_t byte_idx = 0;
    for (uint8_t y = 0; y < 6; y++)
    {
        for (uint8_t x = 0; x < 6; x += 2)
        {
            uint8_t b = pgm_read_byte(&sprite[byte_idx++]);
            uint8_t p1 = b >> 4;
            uint8_t p2 = b & 0x0F;

            // Pixel 1
            if (p1 == 0)
                TFT_spiWrite16(get_candy_kingdom_pixel(px + x, py + y));
            else
                TFT_spiWrite16(pgm_read_word(&jake_palette[p1]));

            // Pixel 2
            if (p2 == 0)
                TFT_spiWrite16(get_candy_kingdom_pixel(px + x + 1, py + y));
            else
                TFT_spiWrite16(pgm_read_word(&jake_palette[p2]));
        }
    }
    TFT_CS_HIGH();
}

// Draw high-detail Candy item on the board
static void draw_candy(int8_t gx, int8_t gy, uint8_t type)
{
    const uint8_t *sprite;
    switch (type)
    {
    case 0:
        sprite = candy_peppermint;
        break;
    case 1:
        sprite = candy_gumdrop;
        break;
    case 2:
        sprite = candy_lemon;
        break;
    case 3:
        sprite = candy_jellybean;
        break;
    case 4:
    default:
        sprite = candy_raspberry;
        break;
    }

    uint8_t px = BOARD_OFFSET_X + (gx * GRID_SIZE);
    uint8_t py = BOARD_OFFSET_Y + (gy * GRID_SIZE);

    TFT_SetAddrWindow(px, py, px + 5, py + 5);
    TFT_CS_LOW();
    TFT_DC_HIGH();

    uint8_t byte_idx = 0;
    for (uint8_t y = 0; y < 6; y++)
    {
        for (uint8_t x = 0; x < 6; x += 2)
        {
            uint8_t b = pgm_read_byte(&sprite[byte_idx++]);
            uint8_t p1 = b >> 4;
            uint8_t p2 = b & 0x0F;

            // Pixel 1
            if (p1 == 0)
                TFT_spiWrite16(get_candy_kingdom_pixel(px + x, py + y));
            else
                TFT_spiWrite16(pgm_read_word(&candy_palette[p1]));

            // Pixel 2
            if (p2 == 0)
                TFT_spiWrite16(get_candy_kingdom_pixel(px + x + 1, py + y));
            else
                TFT_spiWrite16(pgm_read_word(&candy_palette[p2]));
        }
    }
    TFT_CS_HIGH();
}

static void draw_candy_on_bg(int16_t px, int16_t py, uint8_t type, uint16_t bg_color)
{
    const uint8_t *sprite;
    switch (type)
    {
    case 0:
        sprite = candy_peppermint;
        break;
    case 1:
        sprite = candy_gumdrop;
        break;
    case 2:
        sprite = candy_lemon;
        break;
    case 3:
        sprite = candy_jellybean;
        break;
    case 4:
    default:
        sprite = candy_raspberry;
        break;
    }

    TFT_SetAddrWindow((uint8_t)px, (uint8_t)py, (uint8_t)(px + 5), (uint8_t)(py + 5));
    TFT_CS_LOW();
    TFT_DC_HIGH();

    uint8_t byte_idx = 0;
    for (uint8_t y = 0; y < 6; y++)
    {
        for (uint8_t x = 0; x < 6; x += 2)
        {
            uint8_t b = pgm_read_byte(&sprite[byte_idx++]);
            uint8_t p1 = b >> 4;
            uint8_t p2 = b & 0x0F;

            if (p1 == 0)
                TFT_spiWrite16(bg_color);
            else
                TFT_spiWrite16(pgm_read_word(&candy_palette[p1]));

            if (p2 == 0)
                TFT_spiWrite16(bg_color);
            else
                TFT_spiWrite16(pgm_read_word(&candy_palette[p2]));
        }
    }
    TFT_CS_HIGH();
}

static void spawn_food(void)
{
    bool on_snake;
    do
    {
        on_snake = false;
        food.x = rand() % BOARD_WIDTH;
        food.y = rand() % BOARD_HEIGHT;

        for (uint8_t i = 0; i < snake_length; i++)
        {
            if (snake[i].x == food.x && snake[i].y == food.y)
            {
                on_snake = true;
                break;
            }
        }
    } while (on_snake);

    food_type = rand() % 5;
}

static void snake_init(void)
{
    snake_length = 3;
    current_dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;
    score = 0;
    move_interval = 224;
    move_interval = 224;

    snake[0].x = 10;
    snake[0].y = 8;
    snake[1].x = 9;
    snake[1].y = 8;
    snake[2].x = 8;
    snake[2].y = 8;

    spawn_food();
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

void play_snake(void)
{
    uint8_t again = true;
    while (again)
    {
        TFT_FillScreen(TFT_BLACK);
        draw_candy_kingdom_full_scene();
        snake_init();

        char s_b[16];
        uint16_t last_s = 0;

        // Initial render: candy and full Jake
        draw_candy(food.x, food.y, food_type);
        for (uint8_t i = 1; i < snake_length; i++)
        {
            draw_jake_segment(snake[i].x, snake[i].y, (i == snake_length - 1));
        }
        draw_jake_head(snake[0].x, snake[0].y, current_dir);

        // Ready screen: wait for player to press START
        draw_string_centered(58, "PRESS START", 0xFDE0, CK_BANNER_MAGENTA, 1);

        while (!get_key(START))
        {
            update_music();
            timer_update();
            if (get_key(BACK))
            {
                again = false;
                break;
            }
        }

        if (!again)
            break;

        // Clear "PRESS START" text (drawn at x: 47..112, y: 58..65) by restoring exact pixel region
        TFT_SetAddrWindow(44, 56, 115, 67);
        TFT_CS_LOW();
        TFT_DC_HIGH();
        for (uint8_t y = 56; y <= 67; y++)
        {
            for (uint8_t x = 44; x <= 115; x++)
            {
                TFT_spiWrite16(get_candy_kingdom_pixel(x, y));
            }
        }
        TFT_CS_HIGH();
        // Ensure candy and Jake remain clean if overlapped
        draw_candy(food.x, food.y, food_type);
        for (uint8_t i = 1; i < snake_length; i++)
        {
            draw_jake_segment(snake[i].x, snake[i].y, (i == snake_length - 1));
        }
        draw_jake_head(snake[0].x, snake[0].y, current_dir);

        uint32_t last_move_time = millis();
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

                // Erase tail segment by restoring its Candy Kingdom background scenery
                restore_candy_kingdom_cell(snake[snake_length - 1].x, snake[snake_length - 1].y);

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

                // Boundary check
                if (snake[0].x < 0 || snake[0].x >= BOARD_WIDTH ||
                    snake[0].y < 0 || snake[0].y >= BOARD_HEIGHT)
                {
                    end = true;
                    break;
                }

                // Self collision check
                bool self_collision = false;
                for (int i = 1; i < snake_length; i++)
                {
                    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
                    {
                        self_collision = true;
                        break;
                    }
                }
                if (self_collision)
                {
                    end = true;
                    break;
                }

                // Consuming candy: Jake expands!
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

                    play_sfx(sfx_snake_eat);
                    spawn_food();
                    draw_candy(food.x, food.y, food_type);
                }

                // Score display update in HUD banner
                if (score != last_s)
                {
                    last_s = score;
                    itoa(score, s_b, 10);
                    TFT_DrawString(60, 3, "    ", CK_BANNER_MAGENTA, CK_BANNER_MAGENTA, 1);
                    TFT_DrawString(60, 3, s_b, 0xFFE0, CK_BANNER_MAGENTA, 1);
                }

                // Draw Jake: only update tail, body transition, and new head
                draw_jake_segment(snake[snake_length - 1].x, snake[snake_length - 1].y, true);
                if (snake_length > 2)
                {
                    draw_jake_segment(snake[1].x, snake[1].y, false);
                }
                draw_jake_head(snake[0].x, snake[0].y, current_dir);

                // Ensure candy remains visible
                draw_candy(food.x, food.y, food_type);
            }
        }

        if (again)
        {
            play_sfx(sfx_gameover_fast);
            TFT_FillScreen(CK_BANNER_MAGENTA);

            // Candy Kingdom Royal Dialog Card (144x104, centered on 160x128 screen)
            TFT_FillRect(8, 12, 144, 104, CK_CASTLE_PINK);

            // Double border frame (gold outer, bubblegum inner)
            TFT_DrawHLine_Fast(8, 12, 144, CK_BANNER_BORDER);
            TFT_DrawHLine_Fast(8, 115, 144, CK_BANNER_BORDER);
            TFT_DrawVLine_Fast(8, 12, 104, CK_BANNER_BORDER);
            TFT_DrawVLine_Fast(151, 12, 104, CK_BANNER_BORDER);

            TFT_DrawHLine_Fast(10, 14, 140, 0xFDE0);
            TFT_DrawHLine_Fast(10, 113, 140, 0xFDE0);
            TFT_DrawVLine_Fast(10, 14, 100, 0xFDE0);
            TFT_DrawVLine_Fast(149, 14, 100, 0xFDE0);

            // Header Banner ribbon
            TFT_FillRect(11, 15, 138, 26, CK_BANNER_MAGENTA);
            TFT_DrawHLine_Fast(11, 41, 138, CK_BANNER_BORDER);

            // 1. Title: "SUGAR CRASH!"
            draw_string_centered(24, "SUGAR CRASH!", 0xFDE0, CK_BANNER_MAGENTA, 1);
            TFT_DrawString(43, 24, "SUGAR CRASH!", 0xFFFF, CK_BANNER_MAGENTA, 1);

            // 2. Score section with pill badge
            TFT_FillRect(28, 48, 104, 18, 0xFFFF);
            TFT_DrawHLine_Fast(28, 48, 104, CK_BANNER_BORDER);
            TFT_DrawHLine_Fast(28, 65, 104, CK_BANNER_BORDER);
            TFT_DrawVLine_Fast(28, 48, 18, CK_BANNER_BORDER);
            TFT_DrawVLine_Fast(131, 48, 18, CK_BANNER_BORDER);

            sprintf(s_b, "Score: %u", score);
            draw_string_centered(53, s_b, CK_BANNER_MAGENTA, 0xFFFF, 1);

            // Decorative candy icons beside the score badge
            draw_candy_on_bg(16, 54, 0, CK_CASTLE_PINK);
            draw_candy_on_bg(138, 54, 1, CK_CASTLE_PINK);

            // 3. Action Buttons with distinct badges
            // START Button
            TFT_FillRect(24, 73, 112, 16, 0x1B64);
            TFT_DrawHLine_Fast(24, 73, 112, 0x3FE0);
            TFT_DrawHLine_Fast(24, 88, 112, 0x3FE0);
            TFT_DrawVLine_Fast(24, 73, 16, 0x3FE0);
            TFT_DrawVLine_Fast(135, 73, 16, 0x3FE0);
            draw_string_centered(77, "START: Play Again", 0xFFFF, 0x1B64, 1);

            // BACK Button
            TFT_FillRect(24, 93, 112, 16, 0x780F);
            TFT_DrawHLine_Fast(24, 93, 112, CK_BANNER_BORDER);
            TFT_DrawHLine_Fast(24, 108, 112, CK_BANNER_BORDER);
            TFT_DrawVLine_Fast(24, 93, 16, CK_BANNER_BORDER);
            TFT_DrawVLine_Fast(135, 93, 16, CK_BANNER_BORDER);
            draw_string_centered(97, "BACK: Main Menu", 0xFFFF, 0x780F, 1);

            while (true)
            {
                update_music();
                timer_update();
                if (get_key(START))
                {
                    again = true;
                    break;
                }
                if (get_key(BACK))
                {
                    again = false;
                    break;
                }
            }
        }
    }
    stop_song();
}
