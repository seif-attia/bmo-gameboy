#pragma once

#include <avr/io.h>
#include <stdint.h>
#include <BIT_MATH.h>

#define CONTROLLER_PORT PORTC
#define CONTROLLER_PIN PINC
#define CONTROLLER_DDR DDRC

#define RIGHT PC0
#define LEFT PC1
#define UP PC2
#define DOWN PC3
#define START PC4
#define BACK PC5

// IR Remote button hex codes (Samsung NEC protocol)
#define IR_KEY_UP       0xE0E006F9UL
#define IR_KEY_DOWN     0xE0E08679UL
#define IR_KEY_LEFT     0xE0E0A659UL
#define IR_KEY_RIGHT    0xE0E046B9UL
#define IR_KEY_RETURN   0xE0E01AE5UL
#define IR_KEY_START    0xE0E016E9UL

// Button macros compatible with game code
#define UP_BUTTON       IR_KEY_UP
#define DOWN_BUTTON     IR_KEY_DOWN
#define LEFT_BUTTON     IR_KEY_LEFT
#define RIGHT_BUTTON    IR_KEY_RIGHT
#define RETURN_BUTTON   IR_KEY_RETURN
#define START_BUTTON    IR_KEY_START

/* Button pins are active low because we use the internal pullups of the pins by setting the DDR as INPUT and making the PORT Pins HIGH */

/**
 * @brief Initializes the DDR direction for the controller keys (PORTC) and IR driver receiver pin
 *
 * @note Put it before the main loop
 */
void controller_init(void);

/**
 * @brief Checks if a key was pressed (works with both physical buttons and IR remote)
 *
 * @param key takes key pin number (UP, DOWN, LEFT, RIGHT, START, BACK)
 * @return uint8_t Returns 0 (False) or 1 (True) based on whether the key was pressed
 */
uint8_t get_key(uint8_t key);

/**
 * @brief Reads the raw button hex code from the IR remote (or 0 if none/idle)
 *
 * @return uint32_t UP_BUTTON, DOWN_BUTTON, LEFT_BUTTON, RIGHT_BUTTON, RETURN_BUTTON, START_BUTTON, or 0
 */
uint32_t get_key_signal(void);