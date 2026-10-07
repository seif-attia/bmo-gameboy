#include <avr/io.h>

// Define SPI Pins
#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define PIN_MOSI PB3 // Arduino Pin 11
#define PIN_MISO PB4 // Arduino Pin 12
#define PIN_SCK PB5  // Arduino Pin 13
#define PIN_CS PB2   // Arduino Pin 10

/**
 * Initializes the ATmega328P SPI hardware in Master Mode
 w ah btfr2 feeh slave mode as i explained in my simple document
 */

void SPI_Init(void)
{

    // 1. Set MOSI, SCK, and SS as output pins; MISO stays input

    SPI_DDR |= (1 << PIN_MOSI) | (1 << PIN_SCK) | (1 << PIN_CS);

    // 2. Drive Chip Select (CS) HIGH initially (device idle)

    SPI_PORT |= (1 << PIN_CS);

    // 3. Enable SPI, set as Master, and configure clock speed (F_CPU / 16)
    // SPCR: SPI Control Register
    // SPE: SPI Enable, MSTR: Master Mode, SPR0: Clock Rate Select 0

    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

    // Doubles SPI speed
    SPSR |= (1 << SPI2X);
}

/**
 * Transmits one byte of data and simultaneously receives one byte
 * @param data The byte to be transmitted
 * @return The byte received from the peripheral device
 */

uint8_t SPI_Transfer(uint8_t data)
{

    // 1. Write data to the SPI Data Register to initiate transmission

    SPDR = data;

    // 2. Wait until transmission completes
    // SPSR: SPI Status Register
    // SPIF: SPI Interrupt Flag (turns 1 when 8-bit transfer finishes)

    while (!(SPSR & (1 << SPIF)))
    {

        // Do nothing / busy-wait loop
    }

    // 3. Return the received data sitting in the buffer

    return SPDR;
}

/**
 * Selects the peripheral device by pulling CS low
 */
void SPI_Select(void)
{
    SPI_PORT &= ~(1 << PIN_CS);
}

/**
 * Deselects the peripheral device by pulling CS high
 */

void SPI_Deselect(void)
{
    SPI_PORT |= (1 << PIN_CS);
}