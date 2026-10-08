#include "SPI.h"

void SPI_Init(void)
{
    // 1. Set MOSI, SCK, and CS as output pins
    SPI_DDR |= (1 << PIN_MOSI) | (1 << PIN_SCK) | (1 << PIN_CS);

    // 2. Drive CS HIGH initially
    SPI_PORT |= (1 << PIN_CS);

    // 3. Enable SPI, set as Master, Maximum Speed (f_osc / 2 = 8 MHz at 16 MHz CPU)
    // SPR0 = 0, SPR1 = 0 combined with SPI2X = 1 sets clock to f_osc / 2
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR |= (1 << SPI2X);
}

uint8_t SPI_Transfer(uint8_t data)
{
    SPDR = data;
    while (!(SPSR & (1 << SPIF)))
        ;
    return SPDR;
}

void SPI_Select(void)
{
    SPI_PORT &= ~(1 << PIN_CS);
}

void SPI_Deselect(void)
{
    SPI_PORT |= (1 << PIN_CS);
}