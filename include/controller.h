#pragma once

#include <avr/io.h>
#include <stdint.h>
#include "BIT_MATH.h"

#define CONTROLLER_PORT PORTC
#define CONTROLLER_PIN PINC
#define CONTROLLER_DDR DDRC

#define RIGHT PC0
#define LEFT PC1
#define UP PC2
#define DOWN PC3
#define START PC4
#define BACK PC5

/* Button pins are active low because we use the internal pullups of the pins by setting the DDR as INPUT and making the PORT Pins HIGH */

/**
 * @brief Initializes the DDR direction and pull-ups for the controller keys (PORTC).
 */
void controller_init(void);

/**
 * @brief Checks if a key was pressed (returns 1 on initial press edge, 0 otherwise).
 *
 * @param key takes key pin number (UP, DOWN, LEFT, RIGHT, START, BACK)
 * @return uint8_t Returns 0 (False) or 1 (True) based on whether the key was pressed
 */
uint8_t get_key(uint8_t key);