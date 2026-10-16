#include "tictactoe.h"
#include "controller.h"
#include "TFT.h"
#include "timer.h"
#include "music.h"
#include <stdio.h>
#include <stdbool.h>

// Ice Kingdom Theme Colors (RGB565)
#define COLOR_SNOW_BG       0xCE7F   // Crisp arctic pale blue-white snow (RGB 200, 206, 255)
#define COLOR_SNOW_DRIFT    0xE73F   // Glowing pure snow highlights / mounds (RGB 228, 230, 255)
#define COLOR_SNOW_SHADOW   0x9D9E   // Chilly periwinkle blue shadow (RGB 156, 178, 240)
#define COLOR_ICE_DEEP      0x1339   // Dark glinting glacial deep blue
#define COLOR_ICE_CORE      0x4E7F   // Crystalline cyan ice core (RGB 74, 203, 255)
#define COLOR_ICE_HIGHLIGHT 0xDFFF   // Gleaming translucent frost / white reflection (RGB 220, 254, 255)
#define COLOR_ICE_SHADOW    0x2314   // Ice shard drop shadow

// Ice King Palette
#define COLOR_IK_CROWN      0xFDE0   // Gold crown
#define COLOR_IK_RUBY       0xF800   // Red crown rubies
#define COLOR_IK_SKIN       0x5EBF   // Chilly cyan blue face skin
#define COLOR_IK_BEARD      0xFFFF   // Flowing white beard & eyebrows
#define COLOR_IK_BEARD_SHD  0xBDD7   // Beard folds / shadow
#define COLOR_IK_NOSE       0x341F   // Sharp pointed blue nose
#define COLOR_IK_EYE        0x0000   // Eye dots
#define COLOR_IK_ROBE       0x1A19   // Dark royal blue robe at base

// Gunter Palette
#define COLOR_GUNTER_BODY   0x0841   // Inky penguin black feathers
#define COLOR_GUNTER_BELLY  0xFFFF   // Crisp white penguin belly
#define COLOR_GUNTER_EYE_W  0xFFFF   // Big white round eye
#define COLOR_GUNTER_EYE_P  0x0000   // Big black pupil
#define COLOR_GUNTER_BEAK   0xFD00   // Orange-yellow beak
#define COLOR_GUNTER_CHEEK  0xFCC0   // Cute little cheek blush / highlight

// Board Layout & Grid Dimensions
#define BOARD_OFFSET_X      5
#define BOARD_OFFSET_Y      10
#define CELL_SIZE           36
#define GRID_LINE_WIDTH     4        // Ice shard width

#define BOARD_TOTAL_W       (3 * CELL_SIZE + 2 * GRID_LINE_WIDTH) // 116 px
#define BOARD_TOTAL_H       (3 * CELL_SIZE + 2 * GRID_LINE_WIDTH) // 116 px

static uint16_t score_ik = 0;     // Ice King (Player 1)
static uint16_t score_gunter = 0; // Gunter (Player 2)

static uint8_t board[3][3];       // 0: empty, 1: Ice King, 2: Gunter
static uint8_t cursor_r = 1;
static uint8_t cursor_c = 1;
static uint8_t current_turn = 1;  // 1: Ice King, 2: Gunter (Ice King always starts)

// Draw atmospheric snowy background with drifts, flakes, and icy sparkles
static void draw_snowy_background(void)
{
    // Fill full screen with crisp arctic snow base
    TFT_FillScreen(COLOR_SNOW_BG);

    // Subtle undulating snow drifts across the screen
    for (uint8_t y = 0; y < TFT_HEIGHT; y += 16)
    {
        for (uint8_t x = 0; x < TFT_WIDTH; x += 12)
        {
            uint8_t offset = ((x + y * 3) % 7);
            if (offset == 0)
            {
                TFT_DrawPixel(x, y, COLOR_SNOW_DRIFT);
                TFT_DrawPixel(x + 1, y, COLOR_SNOW_DRIFT);
            }
            else if (offset == 3)
            {
                TFT_DrawPixel(x, y, COLOR_SNOW_SHADOW);
            }
        }
    }

    // Handcrafted sparkling snowflakes floating in the air
    const uint8_t flakes[][2] = {
        {12, 6}, {45, 4}, {82, 8}, {118, 5}, {145, 12},
        {18, 40}, {100, 35}, {130, 26}, {150, 48},
        {8, 80}, {75, 78}, {126, 68}, {140, 90},
        {22, 115}, {60, 122}, {95, 118}, {135, 120}, {152, 114}
    };
    for (uint8_t i = 0; i < sizeof(flakes) / sizeof(flakes[0]); i++)
    {
        uint8_t fx = flakes[i][0];
        uint8_t fy = flakes[i][1];
        // 4-point snowflake glint
        TFT_DrawPixel(fx, fy, TFT_WHITE);
        TFT_DrawPixel(fx - 1, fy, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(fx + 1, fy, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(fx, fy - 1, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(fx, fy + 1, COLOR_ICE_HIGHLIGHT);
    }
}

// Draw realistic, faceted crystalline ice shards that make up the grid lines
static void draw_ice_shards_grid(void)
{
    uint8_t vx1 = BOARD_OFFSET_X + CELL_SIZE;
    uint8_t vx2 = BOARD_OFFSET_X + CELL_SIZE * 2 + GRID_LINE_WIDTH;
    uint8_t hy1 = BOARD_OFFSET_Y + CELL_SIZE;
    uint8_t hy2 = BOARD_OFFSET_Y + CELL_SIZE * 2 + GRID_LINE_WIDTH;

    // Vertical Ice Shards
    uint8_t v_shards[2] = {vx1, vx2};
    for (uint8_t i = 0; i < 2; i++)
    {
        uint8_t sx = v_shards[i];

        // Draw shadow on right edge of shard for 3D depth
        TFT_DrawVLine_Fast(sx + GRID_LINE_WIDTH, BOARD_OFFSET_Y + 1, BOARD_TOTAL_H, COLOR_ICE_SHADOW);

        // Core ice layers: highlight edge, bright core, deep body, faceted edge
        TFT_DrawVLine_Fast(sx,     BOARD_OFFSET_Y, BOARD_TOTAL_H, COLOR_ICE_HIGHLIGHT);
        TFT_DrawVLine_Fast(sx + 1, BOARD_OFFSET_Y, BOARD_TOTAL_H, COLOR_ICE_CORE);
        TFT_DrawVLine_Fast(sx + 2, BOARD_OFFSET_Y, BOARD_TOTAL_H, COLOR_ICE_DEEP);
        TFT_DrawVLine_Fast(sx + 3, BOARD_OFFSET_Y, BOARD_TOTAL_H, COLOR_ICE_CORE);

        // Crystalline ice crystal facets and glints along the shard
        for (uint8_t y = BOARD_OFFSET_Y + 4; y < BOARD_OFFSET_Y + BOARD_TOTAL_H - 4; y += 12)
        {
            TFT_DrawPixel(sx + 1, y, TFT_WHITE);
            TFT_DrawPixel(sx + 2, y + 1, COLOR_ICE_HIGHLIGHT);
            TFT_DrawPixel(sx, y + 2, COLOR_ICE_DEEP);
        }

        // Sharp pointed icicle tips at top and bottom of each shard
        TFT_DrawPixel(sx + 1, BOARD_OFFSET_Y - 2, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(sx + 2, BOARD_OFFSET_Y - 2, COLOR_ICE_CORE);
        TFT_DrawPixel(sx + 1, BOARD_OFFSET_Y - 1, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(sx + 2, BOARD_OFFSET_Y - 1, COLOR_ICE_CORE);

        TFT_DrawPixel(sx + 1, BOARD_OFFSET_Y + BOARD_TOTAL_H, COLOR_ICE_CORE);
        TFT_DrawPixel(sx + 2, BOARD_OFFSET_Y + BOARD_TOTAL_H, COLOR_ICE_DEEP);
        TFT_DrawPixel(sx + 1, BOARD_OFFSET_Y + BOARD_TOTAL_H + 1, COLOR_ICE_HIGHLIGHT);
    }

    // Horizontal Ice Shards
    uint8_t h_shards[2] = {hy1, hy2};
    for (uint8_t i = 0; i < 2; i++)
    {
        uint8_t sy = h_shards[i];

        // Draw shadow beneath shard for 3D depth
        TFT_DrawHLine_Fast(BOARD_OFFSET_X + 1, sy + GRID_LINE_WIDTH, BOARD_TOTAL_W, COLOR_ICE_SHADOW);

        // Core ice layers
        TFT_DrawHLine_Fast(BOARD_OFFSET_X, sy,     BOARD_TOTAL_W, COLOR_ICE_HIGHLIGHT);
        TFT_DrawHLine_Fast(BOARD_OFFSET_X, sy + 1, BOARD_TOTAL_W, COLOR_ICE_CORE);
        TFT_DrawHLine_Fast(BOARD_OFFSET_X, sy + 2, BOARD_TOTAL_W, COLOR_ICE_DEEP);
        TFT_DrawHLine_Fast(BOARD_OFFSET_X, sy + 3, BOARD_TOTAL_W, COLOR_ICE_CORE);

        // Crystalline facets along horizontal shard
        for (uint8_t x = BOARD_OFFSET_X + 4; x < BOARD_OFFSET_X + BOARD_TOTAL_W - 4; x += 12)
        {
            TFT_DrawPixel(x, sy + 1, TFT_WHITE);
            TFT_DrawPixel(x + 1, sy + 2, COLOR_ICE_HIGHLIGHT);
            TFT_DrawPixel(x + 2, sy, COLOR_ICE_DEEP);
        }

        // Sharp pointed ice tips on left and right
        TFT_DrawPixel(BOARD_OFFSET_X - 2, sy + 1, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(BOARD_OFFSET_X - 1, sy + 1, COLOR_ICE_CORE);
        TFT_DrawPixel(BOARD_OFFSET_X - 1, sy + 2, COLOR_ICE_CORE);

        TFT_DrawPixel(BOARD_OFFSET_X + BOARD_TOTAL_W, sy + 1, COLOR_ICE_CORE);
        TFT_DrawPixel(BOARD_OFFSET_X + BOARD_TOTAL_W + 1, sy + 1, COLOR_ICE_HIGHLIGHT);
        TFT_DrawPixel(BOARD_OFFSET_X + BOARD_TOTAL_W, sy + 2, COLOR_ICE_DEEP);
    }
}

// Side scoreboard panel themed for Ice Kingdom
static void draw_scoreboard(void)
{
    char buf[12];

    // Background backing card for sidebar
    TFT_FillRect(123, 8, 35, 112, 0x11C9); // Deep glacial navy background
    TFT_DrawHLine_Fast(123, 8, 35, COLOR_ICE_HIGHLIGHT);
    TFT_DrawHLine_Fast(123, 119, 35, COLOR_ICE_CORE);
    TFT_DrawVLine_Fast(123, 8, 112, COLOR_ICE_CORE);
    TFT_DrawVLine_Fast(157, 8, 112, COLOR_ICE_DEEP);

    // Title: "T-T-T"
    TFT_DrawString(127, 12, "T-T-T", COLOR_ICE_HIGHLIGHT, 0x11C9, 1);

    // Player 1: Ice King
    TFT_DrawString(125, 27, "IK:", COLOR_IK_SKIN, 0x11C9, 1);
    sprintf(buf, "%u", score_ik);
    TFT_DrawString(143, 27, buf, COLOR_IK_CROWN, 0x11C9, 1);

    // Player 2: Gunter
    TFT_DrawString(125, 42, "GUN:", TFT_WHITE, 0x11C9, 1);
    sprintf(buf, "%u", score_gunter);
    TFT_DrawString(148, 42, buf, COLOR_GUNTER_BEAK, 0x11C9, 1);

    // Turn indicator box
    TFT_DrawHLine_Fast(124, 57, 33, COLOR_ICE_CORE);
    TFT_DrawString(125, 62, "TURN:", COLOR_SNOW_DRIFT, 0x11C9, 1);

    // Mini icon for active turn
    if (current_turn == 1)
    {
        TFT_FillRect(126, 75, 28, 22, 0x11C9);
        // Mini Ice King Crown & face
        TFT_DrawString(126, 78, "ICE", COLOR_IK_SKIN, 0x11C9, 1);
        TFT_DrawString(126, 88, "KING", COLOR_IK_CROWN, 0x11C9, 1);
    }
    else
    {
        TFT_FillRect(126, 75, 28, 22, 0x11C9);
        TFT_DrawString(126, 78, "GUN-", TFT_WHITE, 0x11C9, 1);
        TFT_DrawString(126, 88, "TER!", COLOR_GUNTER_BEAK, 0x11C9, 1);
    }

    TFT_DrawHLine_Fast(124, 103, 33, COLOR_ICE_DEEP);
    TFT_DrawString(128, 107, "BACK", COLOR_ICE_HIGHLIGHT, 0x11C9, 1);
}

// Draw detailed Ice King Head (22x25 px) - Highly Show-Accurate
// - Tall jagged golden crown with prominent tall center spire and 3 large red rubies
// - Long pointed blue chin & face beneath wild bushy white hair and giant zig-zag beard
// - Extremely long iconic crooked sharp nose protruding dramatically down over his beard
// - Menacing/crazy beady dark eyes with furrowed bushy white eyebrows
// - Giant sweeping voluminous beard with side zig-zag whiskers and pointed tip
static void draw_ice_king_head(uint8_t cx, uint8_t cy)
{
    // cx, cy is top-left of sprite (cx = cell_x + 7, cy = cell_y + 5)

    // y+0 .. y+5: Iconic Adventure Time Golden Crown
    // Center tall spire (y+0) and flanking angled side spires (y+1)
    TFT_DrawPixel(cx + 10, cy, COLOR_IK_CROWN);
    TFT_DrawPixel(cx + 11, cy, COLOR_IK_CROWN);
    TFT_DrawPixel(cx + 4, cy + 1, COLOR_IK_CROWN);
    TFT_DrawPixel(cx + 17, cy + 1, COLOR_IK_CROWN);

    TFT_DrawHLine_Fast(cx + 3, cy + 2, 3, COLOR_IK_CROWN);
    TFT_DrawHLine_Fast(cx + 9, cy + 1, 4, COLOR_IK_CROWN);
    TFT_DrawHLine_Fast(cx + 16, cy + 2, 3, COLOR_IK_CROWN);

    // Crown main body band
    TFT_FillRect(cx + 2, cy + 3, 18, 3, COLOR_IK_CROWN);

    // 3 Inset Red Rubies (center diamond ruby is larger, just like the show)
    TFT_DrawPixel(cx + 5, cy + 4, COLOR_IK_RUBY);
    TFT_FillRect(cx + 10, cy + 3, 2, 2, COLOR_IK_RUBY);
    TFT_DrawPixel(cx + 16, cy + 4, COLOR_IK_RUBY);

    // Crown shadow / dark gold outline at base
    TFT_DrawHLine_Fast(cx + 2, cy + 5, 18, 0xB480);

    // y+6 .. y+11: Blue Face, Eyes, Furrowed Brows, and Wild Side Hair
    // Wild white hair puffing out from sides of crown
    TFT_FillRect(cx + 1, cy + 6, 2, 5, COLOR_IK_BEARD);
    TFT_FillRect(cx + 19, cy + 6, 2, 5, COLOR_IK_BEARD);

    // Cyan Blue Face base
    TFT_FillRect(cx + 3, cy + 6, 16, 5, COLOR_IK_SKIN);

    // Angled bushy white eyebrows (gives his signature quirky/crazed look)
    TFT_DrawPixel(cx + 5, cy + 6, COLOR_IK_BEARD);
    TFT_DrawHLine_Fast(cx + 6, cy + 7, 3, COLOR_IK_BEARD);
    TFT_DrawHLine_Fast(cx + 13, cy + 7, 3, COLOR_IK_BEARD);
    TFT_DrawPixel(cx + 16, cy + 6, COLOR_IK_BEARD);

    // Beady black eyes beneath eyebrows
    TFT_DrawPixel(cx + 7, cy + 8, COLOR_IK_EYE);
    TFT_DrawPixel(cx + 14, cy + 8, COLOR_IK_EYE);

    // White eye twinkle
    TFT_DrawPixel(cx + 8, cy + 8, TFT_WHITE);
    TFT_DrawPixel(cx + 15, cy + 8, TFT_WHITE);

    // y+9 .. y+15: The Iconic Super-Long Pointed Nose (protruding down over beard)
    // Nose bridge
    TFT_FillRect(cx + 10, cy + 8, 2, 3, COLOR_IK_NOSE);
    // Upper nose hook
    TFT_DrawPixel(cx + 9, cy + 10, COLOR_IK_NOSE);
    TFT_DrawPixel(cx + 9, cy + 11, COLOR_IK_NOSE);
    // Mid long nose shaft
    TFT_FillRect(cx + 8, cy + 12, 3, 2, COLOR_IK_NOSE);
    // Sharp hook tip pointing down
    TFT_DrawPixel(cx + 7, cy + 14, COLOR_IK_NOSE);
    TFT_DrawPixel(cx + 8, cy + 14, COLOR_IK_NOSE);
    TFT_DrawPixel(cx + 7, cy + 15, COLOR_IK_NOSE); // Needle tip

    // y+11 .. y+24: Giant Signature Spiky Beard (Adventure Time show style)
    // Beard top connects right under his cheekbones and around the long nose
    TFT_FillRect(cx + 1, cy + 11, 8, 3, COLOR_IK_BEARD);
    TFT_FillRect(cx + 13, cy + 11, 8, 3, COLOR_IK_BEARD);

    // Side beard spikes / zig-zags
    TFT_DrawPixel(cx, cy + 12, COLOR_IK_BEARD);
    TFT_DrawPixel(cx, cy + 13, COLOR_IK_BEARD);
    TFT_DrawPixel(cx + 21, cy + 12, COLOR_IK_BEARD);
    TFT_DrawPixel(cx + 21, cy + 13, COLOR_IK_BEARD);

    // Mid voluminous beard
    TFT_FillRect(cx + 1, cy + 14, 20, 4, COLOR_IK_BEARD);
    TFT_DrawPixel(cx, cy + 16, COLOR_IK_BEARD);
    TFT_DrawPixel(cx + 21, cy + 16, COLOR_IK_BEARD);

    // Lower tapered beard layers
    TFT_FillRect(cx + 2, cy + 18, 18, 3, COLOR_IK_BEARD);
    TFT_FillRect(cx + 4, cy + 21, 14, 2, COLOR_IK_BEARD);
    TFT_DrawHLine_Fast(cx + 7, cy + 23, 8, COLOR_IK_BEARD);
    TFT_DrawHLine_Fast(cx + 9, cy + 24, 4, COLOR_IK_BEARD); // Pointed beard tip

    // Show-accurate beard fold line accents (icy periwinkle)
    TFT_DrawVLine_Fast(cx + 4, cy + 14, 5, COLOR_IK_BEARD_SHD);
    TFT_DrawVLine_Fast(cx + 17, cy + 14, 5, COLOR_IK_BEARD_SHD);
    TFT_DrawVLine_Fast(cx + 12, cy + 17, 4, COLOR_IK_BEARD_SHD);
    TFT_DrawVLine_Fast(cx + 8, cy + 19, 3, COLOR_IK_BEARD_SHD);
    TFT_DrawVLine_Fast(cx + 14, cy + 19, 3, COLOR_IK_BEARD_SHD);

    // Dark blue robe peek at bottom corners
    TFT_DrawHLine_Fast(cx + 1, cy + 24, 3, COLOR_IK_ROBE);
    TFT_DrawHLine_Fast(cx + 18, cy + 24, 3, COLOR_IK_ROBE);
}

// Draw detailed Gunter the Penguin Head (22x24 px) centered in cell
// - Classic round black penguin head with side flippers
// - Big round white cartoon eyes with black pupils looking curiously
// - Bright golden-orange beak
// - Soft white belly patch & adorable penguin expression
static void draw_gunter_head(uint8_t cx, uint8_t cy)
{
    // cx, cy is top-left of 22x24 sprite (cx = cell_x + 7, cy = cell_y + 6)

    // y+1 .. y+4: Black rounded head dome
    TFT_FillRect(cx + 5, cy + 1, 11, 2, COLOR_GUNTER_BODY);
    TFT_FillRect(cx + 3, cy + 3, 15, 2, COLOR_GUNTER_BODY);

    // Head highlight
    TFT_DrawHLine_Fast(cx + 7, cy + 1, 7, 0x2124);

    // y+5 .. y+12: Penguin Face, Huge Round Eyes & Beak
    // Black outer cheeks & sides
    TFT_FillRect(cx + 2, cy + 5, 17, 8, COLOR_GUNTER_BODY);

    // Left Giant Cartoon Eye (6x6)
    TFT_FillRect(cx + 4, cy + 5, 6, 6, COLOR_GUNTER_EYE_W);
    // Right Giant Cartoon Eye (6x6)
    TFT_FillRect(cx + 11, cy + 5, 6, 6, COLOR_GUNTER_EYE_W);

    // Thin eye borders
    TFT_DrawHLine_Fast(cx + 4, cy + 4, 6, COLOR_GUNTER_BODY);
    TFT_DrawHLine_Fast(cx + 11, cy + 4, 6, COLOR_GUNTER_BODY);

    // Gunter's big adorable black pupils (curious look)
    TFT_FillRect(cx + 6, cy + 6, 3, 4, COLOR_GUNTER_EYE_P);
    TFT_FillRect(cx + 12, cy + 6, 3, 4, COLOR_GUNTER_EYE_P);

    // White eye reflections / shine
    TFT_DrawPixel(cx + 6, cy + 6, TFT_WHITE);
    TFT_DrawPixel(cx + 12, cy + 6, TFT_WHITE);

    // y+9 .. y+13: Gunter's Bright Orange Beak right between/under eyes
    TFT_FillRect(cx + 8, cy + 9, 5, 3, COLOR_GUNTER_BEAK);
    TFT_DrawHLine_Fast(cx + 9, cy + 12, 3, COLOR_GUNTER_BEAK);
    TFT_DrawPixel(cx + 10, cy + 13, COLOR_GUNTER_BEAK); // Beak tip
    // Beak shine & nostril crease
    TFT_DrawHLine_Fast(cx + 8, cy + 9, 5, 0xFFE0); // Bright yellow highlight
    TFT_DrawPixel(cx + 10, cy + 10, 0xC300); // Beak center mouth slit

    // y+14 .. y+23: Body, White Belly & Wings / Flippers
    // Black body sides & wings
    TFT_FillRect(cx + 1, cy + 13, 19, 8, COLOR_GUNTER_BODY);
    TFT_FillRect(cx + 3, cy + 21, 15, 2, COLOR_GUNTER_BODY);

    // White chest / belly bib
    TFT_FillRect(cx + 6, cy + 14, 9, 8, COLOR_GUNTER_BELLY);
    TFT_FillRect(cx + 7, cy + 22, 7, 2, COLOR_GUNTER_BELLY);

    // Side wing separation contours
    TFT_DrawVLine_Fast(cx + 3, cy + 14, 6, 0x2124);
    TFT_DrawVLine_Fast(cx + 17, cy + 14, 6, 0x2124);

    // Cute orange penguin feet at bottom
    TFT_DrawHLine_Fast(cx + 5, cy + 23, 3, COLOR_GUNTER_BEAK);
    TFT_DrawHLine_Fast(cx + 13, cy + 23, 3, COLOR_GUNTER_BEAK);
}

static void draw_cell(uint8_t r, uint8_t c)
{
    uint8_t cell_x = BOARD_OFFSET_X + c * (CELL_SIZE + GRID_LINE_WIDTH);
    uint8_t cell_y = BOARD_OFFSET_Y + r * (CELL_SIZE + GRID_LINE_WIDTH);

    bool is_selected = (r == cursor_r && c == cursor_c);

    // Chilly cell base background (soft icy tint)
    TFT_FillRect(cell_x + 2, cell_y + 2, CELL_SIZE - 4, CELL_SIZE - 4, COLOR_SNOW_BG);

    // Inner frosty cell highlights
    TFT_DrawHLine_Fast(cell_x + 2, cell_y + 2, CELL_SIZE - 4, COLOR_SNOW_DRIFT);
    TFT_DrawVLine_Fast(cell_x + 2, cell_y + 2, CELL_SIZE - 4, COLOR_SNOW_DRIFT);
    TFT_DrawHLine_Fast(cell_x + 2, cell_y + CELL_SIZE - 3, CELL_SIZE - 4, COLOR_SNOW_SHADOW);
    TFT_DrawVLine_Fast(cell_x + CELL_SIZE - 3, cell_y + 2, CELL_SIZE - 4, COLOR_SNOW_SHADOW);

    // Render piece model in cell if occupied
    uint8_t val = board[r][c];
    if (val == 1) // Ice King
    {
        draw_ice_king_head(cell_x + 7, cell_y + 5);
    }
    else if (val == 2) // Gunter
    {
        draw_gunter_head(cell_x + 7, cell_y + 6);
    }

    // Cursor selection border (gleaming gold/ruby crown for cursor, or ice border)
    if (is_selected)
    {
        // 2px cursor highlight
        TFT_DrawHLine_Fast(cell_x, cell_y, CELL_SIZE, COLOR_IK_CROWN);
        TFT_DrawHLine_Fast(cell_x, cell_y + 1, CELL_SIZE, COLOR_IK_CROWN);
        TFT_DrawHLine_Fast(cell_x, cell_y + CELL_SIZE - 2, CELL_SIZE, COLOR_IK_CROWN);
        TFT_DrawHLine_Fast(cell_x, cell_y + CELL_SIZE - 1, CELL_SIZE, COLOR_IK_CROWN);

        TFT_DrawVLine_Fast(cell_x, cell_y, CELL_SIZE, COLOR_IK_CROWN);
        TFT_DrawVLine_Fast(cell_x + 1, cell_y, CELL_SIZE, COLOR_IK_CROWN);
        TFT_DrawVLine_Fast(cell_x + CELL_SIZE - 2, cell_y, CELL_SIZE, COLOR_IK_CROWN);
        TFT_DrawVLine_Fast(cell_x + CELL_SIZE - 1, cell_y, CELL_SIZE, COLOR_IK_CROWN);

        // Corner ruby accents
        TFT_DrawPixel(cell_x, cell_y, COLOR_IK_RUBY);
        TFT_DrawPixel(cell_x + CELL_SIZE - 1, cell_y, COLOR_IK_RUBY);
        TFT_DrawPixel(cell_x, cell_y + CELL_SIZE - 1, COLOR_IK_RUBY);
        TFT_DrawPixel(cell_x + CELL_SIZE - 1, cell_y + CELL_SIZE - 1, COLOR_IK_RUBY);
    }
    else
    {
        // Subtle ice border
        TFT_DrawHLine_Fast(cell_x, cell_y, CELL_SIZE, COLOR_ICE_DEEP);
        TFT_DrawHLine_Fast(cell_x, cell_y + 1, CELL_SIZE, COLOR_SNOW_SHADOW);
        TFT_DrawHLine_Fast(cell_x, cell_y + CELL_SIZE - 2, CELL_SIZE, COLOR_SNOW_SHADOW);
        TFT_DrawHLine_Fast(cell_x, cell_y + CELL_SIZE - 1, CELL_SIZE, COLOR_ICE_DEEP);

        TFT_DrawVLine_Fast(cell_x, cell_y, CELL_SIZE, COLOR_ICE_DEEP);
        TFT_DrawVLine_Fast(cell_x + 1, cell_y, CELL_SIZE, COLOR_SNOW_SHADOW);
        TFT_DrawVLine_Fast(cell_x + CELL_SIZE - 2, cell_y, CELL_SIZE, COLOR_SNOW_SHADOW);
        TFT_DrawVLine_Fast(cell_x + CELL_SIZE - 1, cell_y, CELL_SIZE, COLOR_ICE_DEEP);
    }
}

static void draw_all_cells(void)
{
    for (uint8_t r = 0; r < 3; r++)
    {
        for (uint8_t c = 0; c < 3; c++)
        {
            draw_cell(r, c);
        }
    }
}

static uint8_t check_winner(void)
{
    // Rows
    for (uint8_t r = 0; r < 3; r++)
    {
        if (board[r][0] != 0 && board[r][0] == board[r][1] && board[r][1] == board[r][2])
            return board[r][0];
    }

    // Columns
    for (uint8_t c = 0; c < 3; c++)
    {
        if (board[0][c] != 0 && board[0][c] == board[1][c] && board[1][c] == board[2][c])
            return board[0][c];
    }

    // Diagonals
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0];

    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2];

    // Check for draw or ongoing
    for (uint8_t r = 0; r < 3; r++)
    {
        for (uint8_t c = 0; c < 3; c++)
        {
            if (board[r][c] == 0)
                return 0; // Still open cells
        }
    }

    return 3; // Draw
}

static void init_round(void)
{
    for (uint8_t r = 0; r < 3; r++)
    {
        for (uint8_t c = 0; c < 3; c++)
        {
            board[r][c] = 0;
        }
    }
    cursor_r = 1;
    cursor_c = 1;
    current_turn = 1; // Ice King always starts

    draw_snowy_background();
    draw_ice_shards_grid();
    draw_scoreboard();
    draw_all_cells();
}

void play_tic_tac_toe(void)
{
    init_round();

    while (true)
    {
        timer_update();
        update_music();

        if (get_key(BACK))
        {
            break;
        }

        uint8_t prev_r = cursor_r;
        uint8_t prev_c = cursor_c;
        bool cursor_moved = false;

        if (get_key(UP))
        {
            if (cursor_r > 0)
            {
                cursor_r--;
                cursor_moved = true;
            }
        }
        else if (get_key(DOWN))
        {
            if (cursor_r < 2)
            {
                cursor_r++;
                cursor_moved = true;
            }
        }
        else if (get_key(LEFT))
        {
            if (cursor_c > 0)
            {
                cursor_c--;
                cursor_moved = true;
            }
        }
        else if (get_key(RIGHT))
        {
            if (cursor_c < 2)
            {
                cursor_c++;
                cursor_moved = true;
            }
        }

        if (cursor_moved)
        {
            draw_cell(prev_r, prev_c);
            draw_cell(cursor_r, cursor_c);
        }

        if (get_key(START))
        {
            if (board[cursor_r][cursor_c] == 0)
            {
                board[cursor_r][cursor_c] = current_turn;
                draw_cell(cursor_r, cursor_c);

                uint8_t outcome = check_winner();
                if (outcome != 0)
                {
                    if (outcome == 1)
                    {
                        score_ik++;
                    }
                    else if (outcome == 2)
                    {
                        score_gunter++;
                    }
                    draw_scoreboard();

                    // Show Themed Ice Banner Box (Centered: 140px wide -> x=10..150, 34px high -> y=47..81)
                    TFT_FillRect(10, 47, 140, 34, 0x08A6); // Glacial midnight navy
                    // Frost & icicle borders
                    TFT_DrawHLine_Fast(10, 47, 140, COLOR_ICE_HIGHLIGHT);
                    TFT_DrawHLine_Fast(10, 80, 140, COLOR_ICE_CORE);
                    TFT_DrawVLine_Fast(10, 47, 34, COLOR_ICE_CORE);
                    TFT_DrawVLine_Fast(149, 47, 34, COLOR_ICE_HIGHLIGHT);

                    if (outcome == 1)
                    {
                        // "ICE KING WINS!" (14 chars * 6 = 84 px -> x = (160 - 84)/2 = 38)
                        TFT_DrawString(38, 53, "ICE KING WINS!", COLOR_IK_SKIN, 0x08A6, 1);
                        // "WENK-O-RAMA!" or "MAGIC EYE!"
                        TFT_DrawString(41, 67, "FROST POWER!", COLOR_IK_CROWN, 0x08A6, 1);
                    }
                    else if (outcome == 2)
                    {
                        // "GUNTER WINS!" (12 chars * 6 = 72 px -> x = (160 - 72)/2 = 44)
                        TFT_DrawString(44, 53, "GUNTER WINS!", TFT_WHITE, 0x08A6, 1);
                        // "WENK WENK WENK!" (15 chars * 6 = 90 px -> x = (160 - 90)/2 = 35)
                        TFT_DrawString(35, 67, "WENK WENK WENK!", COLOR_GUNTER_BEAK, 0x08A6, 1);
                    }
                    else
                    {
                        // "FROZEN DRAW!" (12 chars * 6 = 72 px -> x = (160 - 72)/2 = 44)
                        TFT_DrawString(44, 53, "FROZEN DRAW!", COLOR_ICE_HIGHLIGHT, 0x08A6, 1);
                        TFT_DrawString(50, 67, "BRRRRRRR!", COLOR_SNOW_DRIFT, 0x08A6, 1);
                    }

                    // Wait for START to play again or BACK to return to menu
                    bool exit_game = false;
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

                    init_round();
                    continue;
                }

                // Switch turn
                current_turn = (current_turn == 1) ? 2 : 1;
                draw_scoreboard();
            }
        }
    }
}
