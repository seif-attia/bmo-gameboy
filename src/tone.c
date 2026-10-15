#include "tone.h"
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// Pin mapping: PD3 (OC2B / Arduino Pin 3)
#define TONE_DDR DDRD
#define TONE_PORT PORTD
#define TONE_PIN PD3

void tone_init(void)
{
    // Ensure PD3 is configured as an output
    TONE_DDR |= (1 << TONE_PIN);

    // Reset Timer2 registers to a clean stopped state
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2 = 0;
    OCR2A = 0;
    OCR2B = 0;

    // Pull pin LOW
    TONE_PORT &= ~(1 << TONE_PIN);
}

void tone(uint8_t pin, uint16_t frequency, unsigned long duration)
{
    (void)pin;
    (void)duration;

    if (frequency < 31)
    {
        noTone(0);
        return;
    }

    // Ensure pin direction is output
    TONE_DDR |= (1 << TONE_PIN);

    // Stop Timer2 clock source while reconfiguring
    TCCR2B = 0;

    /*
     * Timer2 Prescalers:
     * Mode 5 (Phase Correct PWM, TOP = OCR2A):
     * Frequency = F_CPU / (2 * Prescaler * OCR2A)
     * ==> OCR2A = F_CPU / (2 * Prescaler * Target_Freq)
     *
     * We iterate from smallest prescaler to largest so that OCR2A
     * is maximized (within 1..255) for highest frequency resolution.
     */
    static const struct {
        uint16_t prescaler;
        uint8_t cs_bits;
    } prescalers[] = {
        { 8,    (1 << CS21) },
        { 32,   (1 << CS21) | (1 << CS20) },
        { 64,   (1 << CS22) },
        { 128,  (1 << CS22) | (1 << CS20) },
        { 256,  (1 << CS22) | (1 << CS21) },
        { 1024, (1 << CS22) | (1 << CS21) | (1 << CS20) }
    };

    uint32_t ocr = 0;
    uint8_t cs_bits = (1 << CS22) | (1 << CS21) | (1 << CS20); // Default to 1024

    for (uint8_t i = 0; i < sizeof(prescalers) / sizeof(prescalers[0]); i++)
    {
        uint32_t val = F_CPU / (2UL * (uint32_t)prescalers[i].prescaler * (uint32_t)frequency);
        if (val <= 255)
        {
            ocr = val;
            cs_bits = prescalers[i].cs_bits;
            break;
        }
    }

    if (ocr > 255)
        ocr = 255;
    if (ocr < 1)
        ocr = 1;

    OCR2A = (uint8_t)ocr;
    // Reduced duty cycle (~12.5%) on OC2B (PD3) to prevent power rail ripple,
    // eliminating SPI latency and game stuttering
    uint8_t duty = (uint8_t)(ocr / 8);
    if (duty < 1)
        duty = 1;
    OCR2B = duty;

    TCNT2 = 0;

    /*
     * Mode 5: Phase Correct PWM with TOP = OCR2A (WGM22:0 = 101)
     * TCCR2A: WGM20 = 1, COM2B1 = 1 (Clear OC2B on match up, set on match down)
     * TCCR2B: WGM22 = 1, CS22:0 = cs_bits
     */
    TCCR2A = (1 << COM2B1) | (1 << WGM20);
    TCCR2B = (1 << WGM22) | cs_bits;
}

void noTone(uint8_t pin)
{
    (void)pin;

    TCCR2B = 0; // Stop Timer2 clock
    TCCR2A = 0; // Disconnect OC2B hardware pin from Timer2

    TONE_PORT &= ~(1 << TONE_PIN); // Pull pin LOW to prevent DC bias
}