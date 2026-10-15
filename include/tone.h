#ifndef TONE_H
#define TONE_H

#include <stdint.h>

/**
 * @brief Initialize Timer2 peripheral and pin direction (PD3 / Pin 3 / OC2B) for hardware tone generation.
 */
void tone_init(void);

/**
 * @brief Drop-in replacement for Arduino tone() using Timer2 Phase Correct PWM on PD3 (OC2B).
 * @param pin Output buzzer pin (PD3 / Pin 3).
 * @param frequency Target tone frequency in Hz (Pass 0 or <31 to stop output).
 * @param duration Unused (kept for compatibility). Duration is managed by the non-blocking state machine.
 */
void tone(uint8_t pin, uint16_t frequency, unsigned long duration);

/**
 * @brief Drop-in replacement for Arduino noTone(). Stops active output on Timer2.
 * @param pin Output buzzer pin.
 */
void noTone(uint8_t pin);

#endif // TONE_H