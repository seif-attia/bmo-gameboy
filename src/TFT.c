#include "TFT.h"
#include "SPI.h"
#include "BIT_MATH.h"
#include <avr/io.h>
#include <util/delay.h>
#include "font5x7.h"

#define delay_ms(x) _delay_ms(x)

// --- Fast Low-Level SPI Primitives ---

// Select/Deselect wrappers for readability
inline static void TFT_CS_LOW(void) { CLR_BIT(PORTB, TFT_CS); }
inline static void TFT_CS_HIGH(void) { SET_BIT(PORTB, TFT_CS); }
inline static void TFT_DC_LOW(void) { CLR_BIT(PORTB, TFT_DC); }
inline static void TFT_DC_HIGH(void) { SET_BIT(PORTB, TFT_DC); }

void TFT_startWrite(void)
{
    TFT_CS_LOW();
}

void TFT_endWrite(void)
{
    TFT_CS_HIGH();
}

// Sends a byte over SPI assuming CS is ALREADY pulled low
static inline void TFT_sendData_Raw(uint8_t data)
{
    TFT_DC_HIGH();
    SPI_Transfer(data);
}

// Sends a command byte over SPI assuming CS is ALREADY pulled low
static inline void TFT_sendCommand_Raw(uint8_t command)
{
    TFT_DC_LOW();
    SPI_Transfer(command);
}

// Public API wrappers
void TFT_sendCommand(uint8_t command)
{
    TFT_CS_LOW();
    TFT_sendCommand_Raw(command);
    TFT_CS_HIGH();
}

void TFT_sendData(uint8_t data)
{
    TFT_CS_LOW();
    TFT_sendData_Raw(data);
    TFT_CS_HIGH();
}

// Optimized Address Window: Single CS transaction for all 11 SPI bytes
void TFT_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    TFT_CS_LOW();

    // Column address set
    TFT_sendCommand_Raw(0x2A);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(x0);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(x1);

    // Row address set
    TFT_sendCommand_Raw(0x2B);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(y0);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(y1);

    // Write to RAM command
    TFT_sendCommand_Raw(RAMWR);

    TFT_CS_HIGH();
}

// Internal raw address window helper (assumes CS is already LOW)
static void TFT_SetAddrWindow_Raw(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    TFT_sendCommand_Raw(0x2A);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(x0);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(x1);

    TFT_sendCommand_Raw(0x2B);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(y0);
    TFT_sendData_Raw(0x00);
    TFT_sendData_Raw(y1);

    TFT_sendCommand_Raw(RAMWR);
}

// --- Initialization ---

void TFT_INIT(void)
{
    SET_BIT(DDRB, TFT_CS);
    SET_BIT(DDRB, TFT_RST);
    SET_BIT(DDRB, TFT_DC);

    SET_BIT(PORTB, TFT_CS);
    SET_BIT(PORTB, TFT_RST);

    delay_ms(50);
    CLR_BIT(PORTB, TFT_RST);
    delay_ms(10);
    SET_BIT(PORTB, TFT_RST);
    delay_ms(120);

    TFT_sendCommand(SWRESET);
    delay_ms(150);

    TFT_sendCommand(SLPOUT);
    delay_ms(120);

    TFT_sendCommand(COLMOD);
    TFT_sendData(0x05); // 16-bit color format

    TFT_sendCommand(MADCTL);
    TFT_sendData(0x60); // Horizontal orientation + RGB

    TFT_sendCommand(FRMCTR1);
    TFT_sendData(0x01);
    TFT_sendData(0x2C);
    TFT_sendData(0x2D);

    TFT_sendCommand(PWCTR1);
    TFT_sendData(0xA2);
    TFT_sendData(0x02);
    TFT_sendData(0x84);

    TFT_sendCommand(PWCTR2);
    TFT_sendData(0xC5);

    TFT_sendCommand(PWCTR3);
    TFT_sendData(0x0A);
    TFT_sendData(0x00);

    TFT_sendCommand(VMCTR1);
    TFT_sendData(0x8A);
    TFT_sendData(0xEE);

    TFT_sendCommand(INVOFF);

    TFT_sendCommand(GMCTRP1);
    TFT_sendData(0x02);
    TFT_sendData(0x1C);
    TFT_sendData(0x07);
    TFT_sendData(0x12);
    TFT_sendData(0x37);
    TFT_sendData(0x32);
    TFT_sendData(0x29);
    TFT_sendData(0x2D);
    TFT_sendData(0x29);
    TFT_sendData(0x25);
    TFT_sendData(0x2B);
    TFT_sendData(0x39);
    TFT_sendData(0x00);
    TFT_sendData(0x01);
    TFT_sendData(0x03);
    TFT_sendData(0x10);

    TFT_sendCommand(GMCTRN1);
    TFT_sendData(0x03);
    TFT_sendData(0x1D);
    TFT_sendData(0x07);
    TFT_sendData(0x06);
    TFT_sendData(0x2E);
    TFT_sendData(0x2C);
    TFT_sendData(0x29);
    TFT_sendData(0x2D);
    TFT_sendData(0x2E);
    TFT_sendData(0x2E);
    TFT_sendData(0x37);
    TFT_sendData(0x3F);
    TFT_sendData(0x00);
    TFT_sendData(0x00);
    TFT_sendData(0x02);
    TFT_sendData(0x10);

    TFT_sendCommand(NORON);
    delay_ms(10);

    TFT_sendCommand(DISPON);
    delay_ms(100);
}

// --- Primitive Drawing Operations ---

void TFT_DrawPixel(uint8_t x, uint8_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
        return;

    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw(x, y, x, y);
    TFT_DC_HIGH();
    TFT_spiWrite16(color);
    TFT_CS_HIGH();
}

void TFT_DrawHLine_Fast(uint8_t x, uint8_t y, uint8_t w, uint16_t color)
{
    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
        return;
    if ((x + w - 1) >= TFT_WIDTH)
        w = TFT_WIDTH - x;

    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw(x, y, x + w - 1, y);
    TFT_DC_HIGH();
    while (w--)
    {
        TFT_spiWrite16(color);
    }
    TFT_CS_HIGH();
}

void TFT_DrawVLine_Fast(uint8_t x, uint8_t y, uint8_t h, uint16_t color)
{
    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
        return;
    if ((y + h - 1) >= TFT_HEIGHT)
        h = TFT_HEIGHT - y;

    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw(x, y, x, y + h - 1);
    TFT_DC_HIGH();
    while (h--)
    {
        TFT_spiWrite16(color);
    }
    TFT_CS_HIGH();
}

void TFT_FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || (x + w) <= 0 || (y + h) <= 0)
        return;

    int16_t x0 = (x < 0) ? 0 : x;
    int16_t y0 = (y < 0) ? 0 : y;
    int16_t x1 = (x + w - 1 >= TFT_WIDTH) ? (TFT_WIDTH - 1) : (x + w - 1);
    int16_t y1 = (y + h - 1 >= TFT_HEIGHT) ? (TFT_HEIGHT - 1) : (y + h - 1);

    uint16_t total_pixels = (uint16_t)(x1 - x0 + 1) * (uint16_t)(y1 - y0 + 1);

    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw((uint8_t)x0, (uint8_t)y0, (uint8_t)x1, (uint8_t)y1);
    TFT_DC_HIGH();
    while (total_pixels--)
    {
        TFT_spiWrite16(color);
    }
    TFT_CS_HIGH();
}

void TFT_FillScreen(uint16_t color)
{
    TFT_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

// --- Shape Functions ---

void TFT_FillCircle_Fast(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // Direct standalone line calls avoid broken nested transactions
    TFT_DrawVLine_Fast(x0, y0 - r, 2 * r + 1, color);
    TFT_DrawHLine_Fast(x0 - r, y0, 2 * r + 1, color);

    while (x < y)
    {
        if (f >= 0)
        {
            TFT_DrawHLine_Fast(x0 - x, y0 + y, 2 * x + 1, color);
            TFT_DrawHLine_Fast(x0 - x, y0 - y, 2 * x + 1, color);
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        TFT_DrawHLine_Fast(x0 - y, y0 + x, 2 * y + 1, color);
        TFT_DrawHLine_Fast(x0 - y, y0 - x, 2 * y + 1, color);
    }
}

// --- Text & Image Rendering ---

void TFT_DrawChar_Fast(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size)
{
    if (c < 32 || c > 126)
        c = '?';

    uint8_t box_w = 6 * size;
    uint8_t box_h = 8 * size;

    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
        return;
    if ((x + box_w - 1) >= TFT_WIDTH)
        box_w = TFT_WIDTH - x;
    if ((y + box_h - 1) >= TFT_HEIGHT)
        box_h = TFT_HEIGHT - y;

    uint16_t font_index = (c - 32) * 5;

    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw(x, y, x + box_w - 1, y + box_h - 1);
    TFT_DC_HIGH();

    for (uint8_t r = 0; r < 8; r++)
    {
        for (uint8_t r_scale = 0; r_scale < size; r_scale++)
        {
            if (((r * size) + r_scale) >= box_h)
                break;

            for (uint8_t c_idx = 0; c_idx < 5; c_idx++)
            {
                uint8_t line = pgm_read_byte(&Font5x7[font_index + c_idx]);
                uint16_t px_color = ((line >> r) & 0x01) ? color : bg_color;

                for (uint8_t c_scale = 0; c_scale < size; c_scale++)
                {
                    if (((c_idx * size) + c_scale) < box_w)
                    {
                        TFT_spiWrite16(px_color);
                    }
                }
            }

            // Spacing column
            for (uint8_t c_scale = 0; c_scale < size; c_scale++)
            {
                if (((5 * size) + c_scale) < box_w)
                {
                    TFT_spiWrite16(bg_color);
                }
            }
        }
    }
    TFT_CS_HIGH();
}

void TFT_DrawString(uint8_t x, uint8_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size)
{
    uint8_t current_x = x;
    uint8_t current_y = y;

    while (*str)
    {
        // Handle newlines
        if (*str == '\n')
        {
            current_x = x;
            current_y += (8 * size);
        }
        else
        {
            // Check right screen margin for auto-wrap
            if ((current_x + (6 * size)) >= TFT_WIDTH)
            {
                current_x = x;
                current_y += (8 * size);
            }
            TFT_DrawChar_Fast(current_x, current_y, *str, color, bg_color, size);
            current_x += (6 * size); // Move cursor right (5 glyph width + 1 spacing)
        }
        str++;
    }
}

void TFT_DrawImageRGB565_Scaled_Clipped(int16_t x, int16_t y, const uint16_t *image,
                                        uint8_t w, uint8_t h, uint8_t scale)
{
    if (scale < 1)
        scale = 1;

    int16_t scaled_w = (int16_t)w * scale;
    int16_t scaled_h = (int16_t)h * scale;

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || (x + scaled_w) <= 0 || (y + scaled_h) <= 0)
        return;

    int16_t x_start = (x < 0) ? 0 : x;
    int16_t y_start = (y < 0) ? 0 : y;
    int16_t x_end = (x + scaled_w - 1 >= TFT_WIDTH) ? (TFT_WIDTH - 1) : (x + scaled_w - 1);
    int16_t y_end = (y + scaled_h - 1 >= TFT_HEIGHT) ? (TFT_HEIGHT - 1) : (y + scaled_h - 1);

    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw((uint8_t)x_start, (uint8_t)y_start, (uint8_t)x_end, (uint8_t)y_end);
    TFT_DC_HIGH();

    // Loop optimized to remove division inside pixel loops
    for (int16_t sy = y_start; sy <= y_end; sy++)
    {
        uint16_t src_y_row = ((sy - y) / scale) * w;

        for (int16_t sx = x_start; sx <= x_end; sx++)
        {
            uint8_t src_x = (sx - x) / scale;
            uint16_t color = pgm_read_word(&image[src_y_row + src_x]);
            TFT_spiWrite16(color);
        }
    }
    TFT_CS_HIGH();
}

void TFT_DrawBitmap1Bit_Scaled_Clipped(int16_t x, int16_t y, const uint8_t *bitmap,
                                       uint8_t w, uint8_t h, uint16_t color,
                                       uint16_t bg_color, uint8_t scale)
{
    if (scale < 1)
        scale = 1;

    // 1. Calculate total scaled footprint on display
    int16_t scaled_w = (int16_t)w * scale;
    int16_t scaled_h = (int16_t)h * scale;

    // 2. Early rejection test (Sprite is completely off-screen)
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || (x + scaled_w) <= 0 || (y + scaled_h) <= 0)
    {
        return;
    }

    // 3. Define visible screen coordinates and clip to display limits
    int16_t x_start = (x < 0) ? 0 : x;
    int16_t y_start = (y < 0) ? 0 : y;
    int16_t x_end = (x + scaled_w - 1 >= TFT_WIDTH) ? (TFT_WIDTH - 1) : (x + scaled_w - 1);
    int16_t y_end = (y + scaled_h - 1 >= TFT_HEIGHT) ? (TFT_HEIGHT - 1) : (y + scaled_h - 1);

    uint8_t bytes_per_row = (w + 7) / 8;

    // 4. Open single SPI transaction for the whole bitmap
    TFT_CS_LOW();
    TFT_SetAddrWindow_Raw((uint8_t)x_start, (uint8_t)y_start, (uint8_t)x_end, (uint8_t)y_end);
    TFT_DC_HIGH();

    // 5. Stream pixels using TFT_spiWrite16
    for (int16_t sy = y_start; sy <= y_end; sy++)
    {
        uint8_t src_y = (sy - y) / scale;
        uint16_t src_y_offset = (uint16_t)src_y * bytes_per_row;

        for (int16_t sx = x_start; sx <= x_end; sx++)
        {
            uint8_t src_x = (sx - x) / scale;

            // Compute byte index and bit mask inside 1-bit array
            uint16_t byte_idx = src_y_offset + (src_x / 8);
            uint8_t bit_mask = 0x80 >> (src_x % 8);

            // Read bit from PROGMEM
            uint8_t is_set = pgm_read_byte(&bitmap[byte_idx]) & bit_mask;
            uint16_t pixel_color = is_set ? color : bg_color;

            // Stream 16-bit color directly
            TFT_spiWrite16(pixel_color);
        }
    }

    TFT_CS_HIGH();
}