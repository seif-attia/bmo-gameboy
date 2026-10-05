#include "TFT.h"
#include "SPI.h"
#include "BIT_MATH.h"
#include <avr/io.h>
#include <util/delay.h>
#include "font5x7.h"

#define delay_ms(x) _delay_ms(x)

// Dear programmer:
// When I wrote this code, only god and
// I knew how it worked.
// Now, only god knows it!
//
// Therefore, if you are trying to optimize
// this routine and it fails (most surely),
// please increase this counter as a
// warning for the next person:
//
// total_hours_wasted_here = 254
//

void TFT_sendCommand(uint8_t command)
{

    CLR_BIT(PORTB, TFT_DC);

    SPI_Select();

    SPI_Transfer(command);

    SPI_Deselect();
}

void TFT_sendData(uint8_t data)
{

    SET_BIT(PORTB, TFT_DC);

    SPI_Select();

    SPI_Transfer(data);

    SPI_Deselect();
}

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
    TFT_sendData(0x05); // make the colors represent in 16 bits

    TFT_sendCommand(MADCTL);
    TFT_sendData(0x60); // for horizontal display +RGB representation , for another options (0x68  for horizontal and BGR),(0xA8 if upside down)

    TFT_sendCommand(FRMCTR1);
    TFT_sendData(0x01);
    TFT_sendData(0x2C);
    TFT_sendData(0x2D); // these values target a refresh rate of ~80 Hz and BTW this is the default mode for the TFT

    TFT_sendCommand(PWCTR1);
    TFT_sendData(0xA2);
    TFT_sendData(0x02);
    TFT_sendData(0x84); // Configures the internally generated high voltage (~12V–15V) used to turn TFT transistors ON, and low voltage ( ~ -7V to -10V) used to turn TFT transistors OFF

    TFT_sendCommand(PWCTR2);
    TFT_sendData(0xC5); // Sets the step-up multiplier ratios for the internal charge pumps

    TFT_sendCommand(PWCTR3);
    TFT_sendData(0x0A);
    TFT_sendData(0x00); // Adjusts the bias current of the internal operational amplifier

    TFT_sendCommand(VMCTR1);
    TFT_sendData(0x8A);
    TFT_sendData(0xEE); // Fine-tunes the exact offset voltage between individual pixel electrodes and the common glass ground layer

    TFT_sendCommand(INVOFF);

    TFT_sendCommand(GMCTRP1); // configure the positive gamma curve

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

    TFT_sendCommand(GMCTRN1); // configure negative gamma curve

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
    delay_ms(10); // Returns the display controller to regular operating mode

    TFT_sendCommand(DISPON);
    delay_ms(100); // Drives power to the frame memory output gates and applies the AC drive voltage to the liquid crystal cells. Without this command, the panel remains blank (or dark) even if pixel data is stored in display memory.
}

// you have to set a drawing boundary on the tft then start drawing

void TFT_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{

    // for columns boundaries
    TFT_sendCommand(0x2A);
    TFT_sendData(0x00);
    TFT_sendData(x0);
    TFT_sendData(0x00);
    TFT_sendData(x1);

    // for rows boundaries
    TFT_sendCommand(0x2B);
    TFT_sendData(0x00);
    TFT_sendData(y0);
    TFT_sendData(0x00);
    TFT_sendData(y1);

    TFT_sendCommand(RAMWR); // to prepare the memory for incoming pixel stream
}

void TFT_DrawPixel(uint8_t x, uint8_t y, uint16_t color)
{

    // boundary guard
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
    {
        return;
    }

    // set our drawing boundary
    TFT_SetAddrWindow(x, y, x, y);

    // send our 16-bit RGB color and because spi can not transfer 16 bit at once , we send it 8 by 8 bits (higher then lower byte concept)
    TFT_sendData((uint8_t)(color >> 8));
    TFT_sendData((uint8_t)(color & 0xFF));
}

void TFT_FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint16_t color)
{

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || (x + w) <= 0 || (y + h) <= 0)
    {
        return;
    }

    // 2. Clip negative starting coordinates
    int16_t x0 = (x < 0) ? 0 : x;
    int16_t y0 = (y < 0) ? 0 : y;

    // 3. Clip positive ending coordinates
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    if (x1 >= TFT_WIDTH)
        x1 = TFT_WIDTH - 1;
    if (y1 >= TFT_HEIGHT)
        y1 = TFT_HEIGHT - 1;

    // 4. Set hardware display boundary window
    TFT_SetAddrWindow((uint8_t)x0, (uint8_t)y0, (uint8_t)x1, (uint8_t)y1);

    // 5. Send pixel stream
    uint8_t high_byte = (uint8_t)(color >> 8);
    uint8_t low_byte = (uint8_t)(color & 0xFF);
    uint16_t total_pixels = (uint16_t)(x1 - x0 + 1) * (uint16_t)(y1 - y0 + 1);

    for (uint16_t i = 0; i < total_pixels; i++)
    {
        TFT_sendData(high_byte);
        TFT_sendData(low_byte);
    }
}

void TFT_FillScreen(uint16_t color)
{
    TFT_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_DrawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color)
{

    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
        return;
    if ((x + w - 1) >= TFT_WIDTH)
        w = TFT_WIDTH - x;

    TFT_SetAddrWindow(x, y, x + w - 1, y);

    uint8_t high_byte = (uint8_t)(color >> 8);
    uint8_t low_byte = (uint8_t)(color & 0xFF);

    for (uint8_t i = 0; i < w; i++)
    {
        TFT_sendData(high_byte);
        TFT_sendData(low_byte);
    }
}

void TFT_DrawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color)
{

    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
        return;
    if ((y + h - 1) >= TFT_HEIGHT)
        h = TFT_HEIGHT - y;

    TFT_SetAddrWindow(x, y, x, y + h - 1);

    uint8_t high_byte = (uint8_t)(color >> 8);
    uint8_t low_byte = (uint8_t)(color & 0xFF);
    for (uint8_t i = 0; i < h; i++)
    {
        TFT_sendData(high_byte);
        TFT_sendData(low_byte);
    }
}

void TFT_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    while (1)
    {
        TFT_DrawPixel((uint8_t)x0, (uint8_t)y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void TFT_DrawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // Core cardinal points
    TFT_DrawPixel(x0, y0 + r, color);
    TFT_DrawPixel(x0, y0 - r, color);
    TFT_DrawPixel(x0 + r, y0, color);
    TFT_DrawPixel(x0 - r, y0, color);

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Plot 8 symmetrical octant points
        TFT_DrawPixel(x0 + x, y0 + y, color);
        TFT_DrawPixel(x0 - x, y0 + y, color);
        TFT_DrawPixel(x0 + x, y0 - y, color);
        TFT_DrawPixel(x0 - x, y0 - y, color);
        TFT_DrawPixel(x0 + y, y0 + x, color);
        TFT_DrawPixel(x0 - y, y0 + x, color);
        TFT_DrawPixel(x0 + y, y0 - x, color);
        TFT_DrawPixel(x0 - y, y0 - x, color);
    }
}

void TFT_FillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color)
{
    // 1. Draw a central vertical line to cover the diameter along the Y-axis
    TFT_DrawVLine(x0, y0 - r, 2 * r + 1, color);
    TFT_DrawHLine(x0 - r, y0, 2 * r + 1, color);

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // 2. Scan through the arc and connect symmetrical left-to-right points
    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Draw horizontal lines across the 4 quadrant pairs
        TFT_DrawHLine(x0 - x, y0 + y, 2 * x + 1, color);
        TFT_DrawHLine(x0 - x, y0 - y, 2 * x + 1, color);
        TFT_DrawHLine(x0 - y, y0 + x, 2 * y + 1, color);
        TFT_DrawHLine(x0 - y, y0 - x, 2 * y + 1, color);
    }
}

void TFT_DrawChar_Fast(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size)
{
    if (c < 32 || c > 126)
        c = '?';

    uint8_t box_w = 6 * size; // 5 glyph columns + 1 spacing column
    uint8_t box_h = 8 * size; // 8 rows tall

    // Clipping check
    if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
        return;
    if ((x + box_w - 1) >= TFT_WIDTH)
        box_w = TFT_WIDTH - x;
    if ((y + box_h - 1) >= TFT_HEIGHT)
        box_h = TFT_HEIGHT - y;

    // 1. Open ONE address window for the entire character bounding box
    TFT_SetAddrWindow(x, y, x + box_w - 1, y + box_h - 1);

    // Pre-calculate color byte-pairs
    uint8_t fg_hi = (uint8_t)(color >> 8);
    uint8_t fg_lo = (uint8_t)(color & 0xFF);
    uint8_t bg_hi = (uint8_t)(bg_color >> 8);
    uint8_t bg_lo = (uint8_t)(bg_color & 0xFF);

    uint16_t font_index = (c - 32) * 5;

    // 2. Stream row-by-row (top to bottom)
    for (uint8_t r = 0; r < 8; r++)
    {
        for (uint8_t r_scale = 0; r_scale < size; r_scale++)
        {
            uint8_t curr_y_offset = (r * size) + r_scale;
            if (curr_y_offset >= box_h)
                break;

            // Stream 5 glyph columns
            for (uint8_t c_idx = 0; c_idx < 5; c_idx++)
            {
                // Read the vertical line slice and extract bit at row 'r'
                uint8_t line = pgm_read_byte(&Font5x7[font_index + c_idx]);
                uint8_t is_set = (line >> r) & 0x01;

                uint8_t hi = is_set ? fg_hi : bg_hi;
                uint8_t lo = is_set ? fg_lo : bg_lo;

                // Stream pixel horizontally for scaled width
                for (uint8_t c_scale = 0; c_scale < size; c_scale++)
                {
                    uint8_t curr_x_offset = (c_idx * size) + c_scale;
                    if (curr_x_offset < box_w)
                    {
                        TFT_sendData(hi);
                        TFT_sendData(lo);
                    }
                }
            }

            // Stream 1 spacing column (always background color)
            for (uint8_t c_scale = 0; c_scale < size; c_scale++)
            {
                uint8_t curr_x_offset = (5 * size) + c_scale;
                if (curr_x_offset < box_w)
                {
                    TFT_sendData(bg_hi);
                    TFT_sendData(bg_lo);
                }
            }
        }
    }
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
            current_x += (6 * size); // Move cursor right (5 width + 1 spacing)
        }
        str++;
    }
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

    // 3. Define visible screen coordinates
    int16_t x_start = x;
    int16_t y_start = y;
    int16_t x_end = x + scaled_w - 1;
    int16_t y_end = y + scaled_h - 1;

    // 4. Clip boundaries to hardware display limits
    if (x_start < 0)
        x_start = 0;
    if (y_start < 0)
        y_start = 0;
    if (x_end >= TFT_WIDTH)
        x_end = TFT_WIDTH - 1;
    if (y_end >= TFT_HEIGHT)
        y_end = TFT_HEIGHT - 1;

    // Pre-calculate byte splits and row stride
    uint8_t fg_hi = (uint8_t)(color >> 8);
    uint8_t fg_lo = (uint8_t)(color & 0xFF);
    uint8_t bg_hi = (uint8_t)(bg_color >> 8);
    uint8_t bg_lo = (uint8_t)(bg_color & 0xFF);
    uint8_t bytes_per_row = (w + 7) / 8;

    // 5. Open display window over visible region
    TFT_SetAddrWindow((uint8_t)x_start, (uint8_t)y_start, (uint8_t)x_end, (uint8_t)y_end);

    // 6. Loop across visible screen pixels
    for (int16_t sy = y_start; sy <= y_end; sy++)
    {
        uint8_t src_y = (sy - y) / scale;

        for (int16_t sx = x_start; sx <= x_end; sx++)
        {
            uint8_t src_x = (sx - x) / scale;

            // Compute byte index and bit position inside 1-bit array
            uint16_t byte_idx = (src_y * bytes_per_row) + (src_x / 8);
            uint8_t bit_mask = 0x80 >> (src_x % 8);

            // Read bit from PROGMEM
            uint8_t is_set = pgm_read_byte(&bitmap[byte_idx]) & bit_mask;
            uint8_t hi = is_set ? fg_hi : bg_hi;
            uint8_t lo = is_set ? fg_lo : bg_lo;

            TFT_sendData(hi);
            TFT_sendData(lo);
        }
    }
}
void TFT_DrawImageRGB565_Scaled_Clipped(int16_t x, int16_t y, const uint16_t *image,
                                        uint8_t w, uint8_t h, uint8_t scale)
{
    if (scale < 1)
        scale = 1;

    // 1. Calculate total scaled footprint on display
    int16_t scaled_w = (int16_t)w * scale;
    int16_t scaled_h = (int16_t)h * scale;

    // 2. Early rejection test (Image is completely off-screen)
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || (x + scaled_w) <= 0 || (y + scaled_h) <= 0)
    {
        return;
    }

    // 3. Define visible screen coordinates
    int16_t x_start = x;
    int16_t y_start = y;
    int16_t x_end = x + scaled_w - 1;
    int16_t y_end = y + scaled_h - 1;

    // 4. Clip boundaries to hardware display limits
    if (x_start < 0)
        x_start = 0;
    if (y_start < 0)
        y_start = 0;
    if (x_end >= TFT_WIDTH)
        x_end = TFT_WIDTH - 1;
    if (y_end >= TFT_HEIGHT)
        y_end = TFT_HEIGHT - 1;

    // 5. Open display window over visible region
    TFT_SetAddrWindow((uint8_t)x_start, (uint8_t)y_start, (uint8_t)x_end, (uint8_t)y_end);

    // 6. Loop across visible screen pixels
    for (int16_t sy = y_start; sy <= y_end; sy++)
    {
        uint8_t src_y = (sy - y) / scale;

        for (int16_t sx = x_start; sx <= x_end; sx++)
        {
            uint8_t src_x = (sx - x) / scale;

            // Calculate 1D element offset in 16-bit array
            uint16_t pixel_idx = ((uint16_t)src_y * w) + src_x;

            // Read 2-byte color word directly from PROGMEM
            uint16_t color = pgm_read_word(&image[pixel_idx]);

            TFT_sendData((uint8_t)(color >> 8));   // High Byte (R+G)
            TFT_sendData((uint8_t)(color & 0xFF)); // Low Byte (G+B)
        }
    }
}