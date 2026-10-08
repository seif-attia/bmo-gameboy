#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>
#include <avr/io.h>

// Pin Definitions
#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define PIN_MOSI PB3
#define PIN_MISO PB4
#define PIN_SCK PB5
#define PIN_CS PB2

// Function Prototypes
void SPI_Init(void);
uint8_t SPI_Transfer(uint8_t data);
void SPI_Select(void);
void SPI_Deselect(void);

// Inline helper for 16-bit color streaming
static inline void TFT_spiWrite16(uint16_t color)
{
    // High Byte
    SPDR = (uint8_t)(color >> 8);
    while (!(SPSR & (1 << SPIF)))
        ;

    // Low Byte
    SPDR = (uint8_t)(color & 0xFF);
    while (!(SPSR & (1 << SPIF)))
        ;
}

#endif /* SPI_H_ */