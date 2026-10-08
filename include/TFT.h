#ifndef TFT_H_
#define TFT_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

// Screen Dimensions
#define TFT_WIDTH 160
#define TFT_HEIGHT 128

// Hardware Pins (PORTB)
#define TFT_CS PB2
#define TFT_RST PB1
#define TFT_DC PB0

// Display Controller Commands
#define SWRESET 0x01 // Software reset
#define SLPOUT 0x11  // Sleep out
#define NORON 0x13   // Normal display mode on
#define INVOFF 0x20  // Display inversion off
#define INVON 0x21   // Display inversion on
#define DISPON 0x29  // Display on
#define CASET 0x2A   // Column address set
#define RASET 0x2B   // Row address set
#define RAMWR 0x2C   // Memory write
#define COLMOD 0x3A  // Interface pixel format (Color mode)
#define MADCTL 0x36  // Memory data access control
#define FRMCTR1 0xB1 // Frame rate control
#define PWCTR1 0xC0  // Power control 1
#define PWCTR2 0xC1  // Power control 2
#define PWCTR3 0xC2  // Power control 3
#define VMCTR1 0xC5  // VCOM control 1
#define GMCTRP1 0xE0 // Positive gamma correction
#define GMCTRN1 0xE1 // Negative gamma correction

// Color Definitions (RGB565 Format)
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_BLUE 0x001F

// Hardware Control Primitives
void TFT_startWrite(void);
void TFT_endWrite(void);
void TFT_sendCommand(uint8_t command);
void TFT_sendData(uint8_t data);

// Core Display Driver API
void TFT_INIT(void);
void TFT_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void TFT_DrawPixel(uint8_t x, uint8_t y, uint16_t color);

// Fast Primitive Operations
void TFT_DrawHLine_Fast(uint8_t x, uint8_t y, uint8_t w, uint16_t color);
void TFT_DrawVLine_Fast(uint8_t x, uint8_t y, uint8_t h, uint16_t color);
void TFT_FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint16_t color);
void TFT_FillScreen(uint16_t color);

// Fast Shapes
void TFT_FillCircle_Fast(int16_t x0, int16_t y0, int16_t r, uint16_t color);

// Text & Rendering Functions
void TFT_DrawChar_Fast(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size);
void TFT_DrawString(uint8_t x, uint8_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size);
void TFT_DrawBitmap1Bit_Scaled_Clipped(int16_t x, int16_t y, const uint8_t *bitmap,
                                       uint8_t w, uint8_t h, uint16_t color,
                                       uint16_t bg_color, uint8_t scale);
void TFT_DrawImageRGB565_Scaled_Clipped(int16_t x, int16_t y, const uint16_t *image,
                                        uint8_t w, uint8_t h, uint8_t scale);

#endif /* TFT_H_ */