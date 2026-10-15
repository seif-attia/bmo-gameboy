#include "IRDriver.h"
#include "timer.h"
#include "global.h"
#include <adc.h>
#include <avr/io.h>

#define IR_PIN 7
#define IR_THRESHOLD 512 // Threshold for ADC reading (0-1023) to differentiate HIGH/LOW

// Fast read for analog pin A7 using a small sample count to preserve microsecond timing
static inline uint8_t ir_read(void) {
    uint16_t adc_val = adc_read_averaged(IR_PIN, 16);      //honaaaaaaaaaaaaaaaaaaaaaaaa
    return (adc_val > IR_THRESHOLD) ? HIGH : LOW;
}

void ir_driver_init(uint8_t pin) {
    // Initialization placeholder if needed
}

// Call right after an edge: returns how long the pin stays at `level`
// in microseconds, or 0 on timeout.
static uint32_t ir_measure(uint8_t level, uint32_t timeout_us) {
    uint32_t start = micros();
    while (ir_read() == level) {
        if (micros() - start > timeout_us) return 0;
    }
    return micros() - start;
}

// Internal raw decoding function
static uint32_t ir_driver_decode_raw(void) {
    // If the line is idle HIGH, wait briefly for a start edge (max 1ms to keep frame timing smooth)
    uint32_t t0 = micros();
    while (ir_read() == HIGH) {
        if (micros() - t0 > 1000) return 0;
    }

    uint32_t mark = ir_measure(LOW, 10000);
    if (mark < 8000) return 0;

    uint32_t space = ir_measure(HIGH, 6000);
    if (space < 4000) return 0;          // rejects repeat codes (~2.25 ms)

    uint32_t data = 0;
    for (int i = 0; i < 32; i++) {
        if (ir_measure(LOW, 1000) == 0) return 0;      // ~562 us mark

        uint32_t s = ir_measure(HIGH, 2500);            // 562 or 1687 us space
        if (s == 0) return 0;

        data >>= 1;                                     // LSB first
        if (s > 1100) data |= 0x80000000UL;
    }

    // Validate command / ~command
    uint8_t cmd     = (data >> 16) & 0xFF;
    uint8_t cmd_inv = (data >> 24) & 0xFF;
    if ((uint8_t)(cmd ^ cmd_inv) != 0xFF) return 0;

    return data;   // addr | ~addr<<8 | cmd<<16 | ~cmd<<24
}

// Main read function with built-in anti-debouncing / repeat prevention
uint32_t ir_driver_read_hex(void) {
    static uint32_t last_hex = 0;
    static uint8_t is_locked = 0;

    uint32_t current_hex = ir_driver_decode_raw();

    // If no button is currently being received (line idle / returning 0)
    if (current_hex == 0) {
        is_locked = 0;
        last_hex = 0;
        return 0;
    }

    // If the same button is held down, block repeat triggers until released
    if (current_hex == last_hex && is_locked) {
        return 0;
    }

    // New unique press detected
    last_hex = current_hex;
    is_locked = 1;

    return current_hex;
}